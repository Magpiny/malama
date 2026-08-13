// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.cpp
// Purpose:     Implements composite layouts, live token budget updates, and toasts
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/chat_panel.hpp"

#include <cstddef>
#include <format>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <wx/activityindicator.h>
#include <wx/anybutton.h>
#include <wx/arrstr.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/event.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/gdicmn.h>
#include <wx/notifmsg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "common/constants.hpp"
#include "config/config_manager.hpp"
#include "engine/markdown/pipeline.hpp"
#include "network/base64.hpp"

namespace malama::ui {

wxDEFINE_EVENT(EVT_USER_PROMPT, wxCommandEvent);
wxDEFINE_EVENT(EVT_CANCEL_GENERATION, wxCommandEvent);

inline constexpr int input_control_box_height = 60;
inline constexpr int main_layout_spacing = 8;
inline constexpr int tray_layout_spacing = 4;
inline constexpr int control_element_padding = 6;
inline constexpr int hex_letter_val = 10;

[[nodiscard]] static auto get_hex_value(char character) noexcept -> int {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + hex_letter_val;
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + hex_letter_val;
    }
    return 0;
}

inline constexpr int nibble_bit_shift = 4;

[[nodiscard]] static auto decode_hex_payload(std::string_view hex_payload) -> std::string {
    std::string decoded_string;
    decoded_string.reserve(hex_payload.size() / 2UZ);

    for (std::size_t index = 0; index + 1 < hex_payload.size(); index += 2) {
        const int high_value = get_hex_value(hex_payload[index]);
        const int low_value = get_hex_value(hex_payload[index + 1]);

        decoded_string.push_back(static_cast<char>((high_value << nibble_bit_shift) | low_value));
    }
    return decoded_string;
}

ChatPanel::ChatPanel(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY),
      m_attachment_engine(std::make_unique<engine::storage::AttachmentManager>()) {
    SetBackgroundColour(wxColour(constants::chatpanel_bg_color));
    setup_layout();
    bind_events();
}

void ChatPanel::setup_layout() noexcept {
    auto *main_sizer = new (std::nothrow) wxBoxSizer(wxVERTICAL);
    m_chat_display_ptr = new (std::nothrow)
        wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);

    m_error_banner_ptr = new (std::nothrow) ErrorBanner(this);
    m_token_budget_bar = new (std::nothrow) TokenBudgetBar(this);

    m_tray_container_ptr = new (std::nothrow) wxPanel(this, wxID_ANY);
    m_tray_sizer_ptr = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);
    m_tray_container_ptr->SetSizer(m_tray_sizer_ptr);
    m_tray_container_ptr->SetBackgroundColour(wxColour(constants::appwindow_bg_color));
    m_tray_container_ptr->Hide();

    auto *input_control_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);

    m_attach_button_ptr =
        new (std::nothrow) wxButton(this, wxID_ANY, wxString::FromUTF8("📎"), wxDefaultPosition,
                                    wxDefaultSize, wxBU_EXACTFIT | wxBORDER_NONE);
    m_prompt_input_ptr = new (std::nothrow)
        wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, input_control_box_height),
                   wxTE_MULTILINE | wxTE_PROCESS_ENTER | wxBORDER_NONE);
    m_spinner_ptr = new (std::nothrow) wxActivityIndicator(this, wxID_ANY);
    m_copy_button_ptr = new (std::nothrow)
        wxButton(this, wxID_ANY, wxString::FromUTF8("📋 Copy"), wxDefaultPosition, wxDefaultSize,
                 wxBU_EXACTFIT | wxBORDER_NONE);
    m_send_button_ptr = new (std::nothrow)
        wxButton(this, wxID_ANY, wxString::FromUTF8("📤 Send"), wxDefaultPosition, wxDefaultSize,
                 wxBU_EXACTFIT | wxBORDER_NONE);

    if (main_sizer == nullptr || m_chat_display_ptr == nullptr || m_error_banner_ptr == nullptr ||
        m_tray_container_ptr == nullptr || input_control_sizer == nullptr ||
        m_token_budget_bar == nullptr || m_attach_button_ptr == nullptr ||
        m_prompt_input_ptr == nullptr || m_spinner_ptr == nullptr || m_copy_button_ptr == nullptr ||
        m_send_button_ptr == nullptr) [[unlikely]] {
        return;
    }

    m_attach_button_ptr->SetToolTip("Attach files or visual assets to current prompt context");
    m_attach_button_ptr->SetCursor(wxCursor(wxCURSOR_HAND));

    m_spinner_ptr->SetToolTip("Forcibly interrupt current generation thread stream loop");
    m_spinner_ptr->SetCursor(wxCursor(wxCURSOR_HAND));

    m_copy_button_ptr->SetToolTip("Copy full response raw text content to clipboard");
    m_copy_button_ptr->SetCursor(wxCursor(wxCURSOR_HAND));

    m_send_button_ptr->SetToolTip("Dispatch collected content buffers directly to local LLM");
    m_send_button_ptr->SetCursor(wxCursor(wxCURSOR_HAND));

    m_spinner_ptr->Hide();
    m_raw_markdown_history = "### System Channel\n`malama v0.3.0-multimodal` ready.\n\n---";
    render_chat_stream();

    input_control_sizer->Add(m_attach_button_ptr, 0, wxALIGN_CENTER_VERTICAL | wxALL,
                             control_element_padding);
    input_control_sizer->Add(m_prompt_input_ptr, 1, wxEXPAND | wxALL, control_element_padding);
    input_control_sizer->Add(m_spinner_ptr, 0, wxALIGN_CENTER_VERTICAL | wxALL,
                             control_element_padding);
    input_control_sizer->Add(m_copy_button_ptr, 0, wxALIGN_CENTER_VERTICAL | wxALL,
                             control_element_padding);
    input_control_sizer->Add(m_send_button_ptr, 0, wxALIGN_CENTER_VERTICAL | wxALL,
                             control_element_padding);

    main_sizer->Add(m_chat_display_ptr, 1, wxEXPAND | wxALL, main_layout_spacing);
    main_sizer->Add(m_error_banner_ptr, 0, wxEXPAND | wxLEFT | wxRIGHT, main_layout_spacing);
    main_sizer->Add(m_token_budget_bar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP,
                    main_layout_spacing);
    main_sizer->Add(m_tray_container_ptr, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,
                    tray_layout_spacing);
    main_sizer->Add(input_control_sizer, 0, wxEXPAND | wxALL, main_layout_spacing);

    SetSizer(main_sizer);
}

[[nodiscard]] auto ChatPanel::get_active_session() const noexcept -> const core::ChatSession & {
    return m_active_session;
}

void ChatPanel::update_token_budget_display() noexcept {
    if (m_token_budget_bar == nullptr) {
        return;
    }

    const std::string current_prompt =
        (m_prompt_input_ptr != nullptr) ? m_prompt_input_ptr->GetValue().ToStdString() : "";

    std::vector<engine::storage::AttachmentInfo> pending_attachments;
    if (m_attachment_engine != nullptr) {
        pending_attachments = m_attachment_engine->GetPendingAttachments();
    }

    // 1. Calculate tokens for stored history + pending attachments + typed input
    std::size_t estimated_tokens = m_token_estimator.estimate_payload_tokens(
        current_prompt, pending_attachments, m_active_session.m_messages);

    // 2. Add tokens from live active response stream
    if (!m_active_response_stream.empty()) {
        estimated_tokens += m_token_estimator.estimate_text_tokens(m_active_response_stream);
    }

    std::size_t num_ctx_limit = m_active_session.m_metadata.m_parameters.m_num_ctx;
    if (num_ctx_limit == 0UZ) {
        num_ctx_limit = 4096UZ;  // Fallback default context window limit
    }

    // 3. Update Visual Budget Bar
    m_token_budget_bar->UpdateUsage(estimated_tokens, num_ctx_limit);

    // 4. Update Error Banner alert
    if (m_error_banner_ptr != nullptr) {
        if (estimated_tokens >= num_ctx_limit) {
            m_error_banner_ptr->ShowAlert(
                std::format(
                    "Context Budget Limit: Token payload (~{}) exceeds active num_ctx ({}).",
                    estimated_tokens, num_ctx_limit),
                BannerState::WARNING);
        } else {
            m_error_banner_ptr->HideAlert();
        }
    }
}

void ChatPanel::append_token(std::string_view token_segment) noexcept {
    m_active_response_stream.append(token_segment.data(), token_segment.size());
    render_chat_stream();
    update_token_budget_display();
}

void ChatPanel::append_user_message(std::string_view message) noexcept {
    // Finalize prior assistant stream into session history
    if (!m_active_response_stream.empty()) {
        m_last_llm_response = m_active_response_stream;
        m_raw_markdown_history += "\n" + m_active_response_stream + "\n\n---";

        m_active_session.m_messages.push_back(
            {.m_role = core::MessageRole::Assistant, .m_content = m_active_response_stream});
        m_active_response_stream.clear();
    }

    std::string lookahead_match = std::string(message);

    if (m_last_processed_prompt == lookahead_match ||
        m_last_processed_prompt.starts_with(lookahead_match)) {
        return;
    }
    m_last_processed_prompt = lookahead_match;

    // Save user message directly into session history
    m_active_session.m_messages.push_back(
        {.m_role = core::MessageRole::User, .m_content = lookahead_match});

    m_raw_markdown_history += "\n\n### 👤 User\n" + lookahead_match + "\n\n### 🤖 malama\n";
    render_chat_stream();
    update_token_budget_display();
}

void ChatPanel::load_history(const core::ChatSession &session) noexcept {
    m_active_session = session;
    m_raw_markdown_history = "### System Channel\n`malama v0.3.0-multimodal` synchronized.\n\n---";
    m_active_response_stream.clear();
    m_last_llm_response.clear();
    m_last_processed_prompt.clear();

    for (const auto &message : session.m_messages) {
        if (message.m_role == core::MessageRole::User) {
            m_raw_markdown_history += "\n\n### 👤 User\n" + message.m_content;
            m_last_processed_prompt = message.m_content;
        } else if (message.m_role == core::MessageRole::Assistant) {
            m_raw_markdown_history += "\n\n### 🤖 malama\n" + message.m_content + "\n\n---";
            m_last_llm_response = message.m_content;
        } else if (message.m_role == core::MessageRole::System) {
            m_raw_markdown_history += "\n\n### ⚙️ System Context\n" + message.m_content + "\n\n---";
        }
    }
    render_chat_stream();
    scroll_to_bottom();
    update_token_budget_display();
}

void ChatPanel::on_prompt_changed(wxCommandEvent &WXUNUSED(event)) noexcept {
    update_token_budget_display();
}

void ChatPanel::on_spinner_mouse_down(wxMouseEvent &WXUNUSED(event)) noexcept {
    stop_spinner();

    wxCommandEvent cancel_event(EVT_CANCEL_GENERATION, GetId());
    cancel_event.SetEventObject(this);
    wxPostEvent(this, cancel_event);

    wxNotificationMessage notification("malama", "Local inference execution loop interrupted.");
    notification.Show(wxICON_INFORMATION);
}

void ChatPanel::on_attach_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    m_error_banner_ptr->HideAlert();

    const wxString support_wildcard =
        "All Supported Assets|"
        "*.txt;*.cpp;*.hpp;*.py;*.json;*.log;*.pdf;*.png;*.jpg;*.jpeg;*.webp;"
        "*.docx;*.odt;*.xlsx;*.ods;*.xml;*.epub";

    wxFileDialog file_dialog(this, "Select File Assets for Ingestion", "", "", support_wildcard,
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

    if (file_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    wxArrayString selected_paths;
    file_dialog.GetPaths(selected_paths);

    for (size_t index = 0; index < selected_paths.GetCount(); ++index) {
        std::string target_path = selected_paths[index].ToStdString();
        auto result = m_attachment_engine->AnalyzeAndAdd(target_path);

        if (!result.has_value()) {
            switch (result.error()) {
                case engine::storage::IngestionError::IMAGE_TOO_LARGE: {
                    m_error_banner_ptr->ShowAlert(
                        "File limit crossed: Vision targets must be under 4MB to protect VRAM.",
                        BannerState::ERROR);
                    break;
                }
                case engine::storage::IngestionError::DOCUMENT_TOO_LARGE: {
                    m_error_banner_ptr->ShowAlert(
                        "File limit crossed: Large document targets must be under 4MB.",
                        BannerState::ERROR);
                    break;
                }
                case engine::storage::IngestionError::FILE_NOT_FOUND:
                case engine::storage::IngestionError::READ_FAULT: {
                    m_error_banner_ptr->ShowAlert(
                        "System I/O Fault: File is unreadable or corrupt.", BannerState::ERROR);
                    break;
                }
                case engine::storage::IngestionError::CONTEXT_OVERFLOW: {
                    m_error_banner_ptr->ShowAlert("Context window limit overflow threat detected.",
                                                  BannerState::WARNING);
                    break;
                }
                case engine::storage::IngestionError::PARSING_FAILED: {
                    m_error_banner_ptr->ShowAlert(
                        "Parsing Failure: File format layout is unreadable or corrupted.",
                        BannerState::ERROR);
                    break;
                }
                case engine::storage::IngestionError::MAX_LIMIT_REACHED: {
                    m_error_banner_ptr->ShowAlert(
                        "Queue Overflow: A maximum of 6 active assets can be loaded concurrently.",
                        BannerState::WARNING);
                    break;
                }
            }
        }
    }

    refresh_attachment_tray();
    update_token_budget_display();
}

void ChatPanel::refresh_attachment_tray() noexcept {
    m_tray_sizer_ptr->Clear(true);
    const auto &attachments = m_attachment_engine->GetPendingAttachments();

    if (attachments.empty()) {
        m_tray_container_ptr->Hide();
        Layout();
        return;
    }

    for (std::size_t index = 0UZ; index < attachments.size(); ++index) {
        const auto &info = attachments[index];
        auto *chip = new (std::nothrow) wxPanel(m_tray_container_ptr, wxID_ANY, wxDefaultPosition,
                                                wxDefaultSize, wxBORDER_SIMPLE);
        auto *chip_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);

        std::string prefix =
            (info.m_type == engine::storage::AttachmentType::IMAGE) ? "🖼️ " : "📄 ";
        auto *label = new (std::nothrow)
            wxStaticText(chip, wxID_ANY, wxString::FromUTF8(prefix + info.m_file_name));
        label->SetForegroundColour(wxColour(constants::chatinput_text_color));

        auto *close_button =
            new (std::nothrow) wxButton(chip, wxID_ANY, wxString::FromUTF8("❌"), wxDefaultPosition,
                                        wxDefaultSize, wxBU_EXACTFIT | wxBORDER_NONE);
        close_button->SetBackgroundColour(wxColour(constants::chatinput_bg_color));
        close_button->SetForegroundColour(wxColour(constants::chatinput_text_color));
        close_button->SetCursor(wxCursor(wxCURSOR_HAND));
        close_button->SetToolTip("Forcibly drop this asset signature from current prompt queue");

        close_button->Bind(wxEVT_BUTTON, [this, index](wxCommandEvent &) {
            m_attachment_engine->RemoveByIndex(index);
            refresh_attachment_tray();
            update_token_budget_display();
        });

        if (chip == nullptr || chip_sizer == nullptr || label == nullptr || close_button == nullptr)
            [[unlikely]] {
            continue;
        }

        chip_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxALL, control_element_padding);
        chip_sizer->Add(close_button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP | wxBOTTOM,
                        control_element_padding);

        chip->SetSizer(chip_sizer);
        chip->SetBackgroundColour(wxColour(constants::chatpanel_bg_color));

        m_tray_sizer_ptr->Add(chip, 0, wxALIGN_CENTER_VERTICAL | wxALL, control_element_padding);
    }

    m_tray_container_ptr->Show();
    Layout();
}

void ChatPanel::on_send_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    const auto raw_prompt = m_prompt_input_ptr->GetValue();
    if (raw_prompt.IsEmpty() && m_attachment_engine->GetPendingAttachments().empty()) {
        return;
    }

    m_prompt_input_ptr->Clear();
    std::string final_processed_prompt = raw_prompt.ToStdString();
    std::string image_layout_indicators;
    std::vector<std::string> base64_images;

    for (const auto &file_info : m_attachment_engine->GetPendingAttachments()) {
        if (file_info.m_type == engine::storage::AttachmentType::TEXT_DOCUMENT) {
            auto text_extraction =
                malama::engine::storage::AttachmentManager::ExtractTextContent(file_info);
            if (text_extraction.has_value()) {
                final_processed_prompt += text_extraction.value();
            } else {
                m_error_banner_ptr->ShowAlert(
                    std::format("Failed to parse document content from: {}", file_info.m_file_name),
                    BannerState::ERROR);
            }
        } else if (file_info.m_type == engine::storage::AttachmentType::IMAGE) {
            image_layout_indicators +=
                std::format("\n\n🖼️ *Attached Image Snapshot: {}*", file_info.m_file_name);

            wxFile image_file(file_info.m_file_path, wxFile::read);
            if (image_file.IsOpened()) {
                const auto file_length = static_cast<std::size_t>(image_file.Length());

                std::vector<char> byte_buffer(file_length);
                image_file.Read(byte_buffer.data(), byte_buffer.size());

                std::string_view zero_copy_view(byte_buffer.data(), byte_buffer.size());
                base64_images.push_back(malama::network::encode_base64(zero_copy_view));
            }
        }
    }

    append_user_message(raw_prompt.ToStdString() + image_layout_indicators);

    if (!base64_images.empty()) {
        malama::network::ImageTransit::SetPendingImages(std::move(base64_images));
    }

    wxCommandEvent custom_event(EVT_USER_PROMPT, GetId());
    custom_event.SetString(
        wxString::FromUTF8(final_processed_prompt.data(), final_processed_prompt.size()));
    wxPostEvent(this, custom_event);

    m_attachment_engine->ClearQueue();
    refresh_attachment_tray();
    update_token_budget_display();
}

void ChatPanel::scroll_to_bottom() noexcept {
    if (m_chat_display_ptr != nullptr) {
        m_chat_display_ptr->Scroll(0, m_chat_display_ptr->GetVirtualSize().GetHeight());
    }
}

void ChatPanel::render_chat_stream() noexcept {
    if (m_chat_display_ptr == nullptr) {
        return;
    }
    auto theme = config::ConfigManager::get_instance().get_config().m_appearance;
    engine::markdown::Pipeline markdown_engine(theme);

    std::string composite = m_raw_markdown_history + "\n" + m_active_response_stream;
    std::string html_body = markdown_engine.process(composite);

    std::string complete_html = std::format(
        "<html><head><meta charset=\"utf-8\"></head><body bgcolor=\"{}\">"
        "<font color=\"{}\" face=\"sans-serif\">{}</font></body></html>",
        theme.m_bg_color, theme.m_text_primary, html_body);

    m_chat_display_ptr->SetPage(wxString::FromUTF8(complete_html.data(), complete_html.size()));
    scroll_to_bottom();
}

auto ChatPanel::get_active_response_stream() const noexcept -> std::string {
    return m_active_response_stream;
}

void ChatPanel::on_prompt_key_down(wxKeyEvent &event) noexcept {
    if (event.GetKeyCode() == WXK_RETURN && !event.ShiftDown()) {
        wxCommandEvent dummy;
        on_send_action(dummy);
    } else {
        event.Skip();
    }
}

void ChatPanel::on_copy_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    if (wxTheClipboard->Open()) {
        wxTheClipboard->Clear();
        std::string target_text =
            m_active_response_stream.empty() ? m_last_llm_response : m_active_response_stream;

        if (!target_text.empty()) {
            auto *data_obj = new (std::nothrow)
                wxTextDataObject(wxString::FromUTF8(target_text.data(), target_text.size()));
            if (data_obj != nullptr) {
                wxTheClipboard->SetData(data_obj);

                wxNotificationMessage system_toast("malama", "Full dialogue response text copied.");
                system_toast.Show(wxICON_INFORMATION);
            }
        }
        wxTheClipboard->Close();
    }
}

void ChatPanel::handle_code_copy(std::string_view hex_payload) noexcept {
    const std::string raw_code = decode_hex_payload(hex_payload);

    if (!wxTheClipboard->Open()) {
        return;
    }

    wxTheClipboard->Clear();

    auto *data_obj =
        new (std::nothrow) wxTextDataObject(wxString::FromUTF8(raw_code.data(), raw_code.size()));

    if (data_obj != nullptr) {
        wxTheClipboard->SetData(data_obj);
        wxNotificationMessage toast("malama", "Code snippet block copied to clipboard.");
        toast.Show(wxICON_INFORMATION);
    }
    wxTheClipboard->Close();
}

void ChatPanel::handle_code_download(std::string_view hex_payload) noexcept {
    const std::string raw_code = decode_hex_payload(hex_payload);

    wxFileDialog save_dialog(this, "Save code block snippet", "", "", "All files (*.*)|*.*",
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (save_dialog.ShowModal() != wxID_OK) {
        return;
    }

    const wxString file_path = save_dialog.GetPath();
    wxFile local_file(file_path, wxFile::write);

    if (local_file.IsOpened()) {
        local_file.Write(raw_code.data(), raw_code.size());
        local_file.Close();

        wxNotificationMessage toast("malama", "Source snippet file exported successfully.");
        toast.Show(wxICON_INFORMATION);
    }
}

void ChatPanel::start_spinner() noexcept {
    if (m_spinner_ptr != nullptr) {
        m_spinner_ptr->Show();
        m_spinner_ptr->Start();
    }
    Layout();
}

void ChatPanel::stop_spinner() noexcept {
    if (m_spinner_ptr != nullptr) {
        m_spinner_ptr->Stop();
        m_spinner_ptr->Hide();
    }
    Layout();
}

void ChatPanel::on_link_clicked(wxHtmlLinkEvent &event) noexcept {
    const wxString link_href = event.GetLinkInfo().GetHref();

    constexpr std::size_t copy_prefix_len = 19UZ;
    constexpr std::size_t down_prefix_len = 23UZ;

    if (link_href.StartsWith("malama://copy_code:")) {
        const wxString hex_segment = link_href.Mid(copy_prefix_len);
        handle_code_copy(hex_segment.ToStdString());
    } else if (link_href.StartsWith("malama://download_code:")) {
        const wxString hex_segment = link_href.Mid(down_prefix_len);
        handle_code_download(hex_segment.ToStdString());
    } else {
        event.Skip();
    }
}

void ChatPanel::bind_events() noexcept {
    m_send_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_send_action, this);
    m_attach_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_attach_action, this);
    m_copy_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_copy_action, this);
    m_prompt_input_ptr->Bind(wxEVT_KEY_DOWN, &ChatPanel::on_prompt_key_down, this);
    m_prompt_input_ptr->Bind(wxEVT_TEXT, &ChatPanel::on_prompt_changed, this);
    m_chat_display_ptr->Bind(wxEVT_HTML_LINK_CLICKED, &ChatPanel::on_link_clicked, this);
    m_spinner_ptr->Bind(wxEVT_LEFT_DOWN, &ChatPanel::on_spinner_mouse_down, this);
}

}  // namespace malama::ui

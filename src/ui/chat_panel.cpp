// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.cpp
// Purpose:     Implements the composite layout, visual asset chips, and safety gates
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#include "ui/chat_panel.hpp"

#include <new>
#include <wx/filedlg.h>
#include <wx/sizer.h>

#include "common/constants.hpp"
#include "config/config_manager.hpp"
#include "engine/markdown/pipeline.hpp"

namespace malama::ui {

wxDEFINE_EVENT(EVT_USER_PROMPT, wxCommandEvent);
wxDEFINE_EVENT(EVT_CANCEL_GENERATION, wxCommandEvent);

ChatPanel::ChatPanel(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY),
      m_attachment_engine(std::make_unique<engine::storage::AttachmentManager>()) {
    SetBackgroundColour(wxColour(std::string(constants::color_dark_maroon)));
    setup_layout();
    bind_events();
}

void ChatPanel::setup_layout() noexcept {
    auto *main_sizer = new (std::nothrow) wxBoxSizer(wxVERTICAL);
    m_chat_display_ptr = new (std::nothrow)
        wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);

    // Inject our inline security and error notification banner
    m_error_banner_ptr = new (std::nothrow) ErrorBanner(this);

    // Setup the isolated Attachment Chip Panel Workspace
    m_tray_container_ptr = new (std::nothrow) wxPanel(this, wxID_ANY);
    m_tray_sizer_ptr = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);
    m_tray_container_ptr->SetSizer(m_tray_sizer_ptr);
    m_tray_container_ptr->SetBackgroundColour(wxColour("#1A0105"));
    m_tray_container_ptr->Hide();

    auto *input_control_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);

    m_attach_button_ptr =
        new (std::nothrow) wxButton(this, wxID_ANY, wxString::FromUTF8("📎"), wxDefaultPosition,
                                    wxDefaultSize, wxBU_EXACTFIT | wxBORDER_NONE);
    m_prompt_input_ptr =
        new (std::nothrow) wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 60),
                                      wxTE_MULTILINE | wxTE_PROCESS_ENTER | wxBORDER_NONE);
    m_spinner_ptr = new (std::nothrow) wxActivityIndicator(this, wxID_ANY);
    m_copy_button_ptr = new (std::nothrow)
        wxButton(this, wxID_ANY, wxString::FromUTF8("📋 Copy"), wxDefaultPosition, wxDefaultSize,
                 wxBU_EXACTFIT | wxBORDER_NONE);
    m_send_button_ptr = new (std::nothrow)
        wxButton(this, wxID_ANY, wxString::FromUTF8("📤 Send"), wxDefaultPosition, wxDefaultSize,
                 wxBU_EXACTFIT | wxBORDER_NONE);

    if (!main_sizer || !m_chat_display_ptr || !m_error_banner_ptr || !m_tray_container_ptr ||
        !input_control_sizer || !m_attach_button_ptr || !m_prompt_input_ptr || !m_spinner_ptr ||
        !m_copy_button_ptr || !m_send_button_ptr) [[unlikely]] {
        return;
    }

    m_spinner_ptr->Hide();
    m_raw_markdown_history = "### System Channel\n`malama v0.2.7-multimodal` ready.\n\n---";
    render_chat_stream();

    input_control_sizer->Add(m_attach_button_ptr, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    input_control_sizer->Add(m_prompt_input_ptr, 1, wxEXPAND | wxALL, 4);
    input_control_sizer->Add(m_spinner_ptr, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    input_control_sizer->Add(m_copy_button_ptr, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    input_control_sizer->Add(m_send_button_ptr, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);

    main_sizer->Add(m_chat_display_ptr, 1, wxEXPAND | wxALL, 6);
    main_sizer->Add(m_error_banner_ptr, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
    main_sizer->Add(m_tray_container_ptr, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
    main_sizer->Add(input_control_sizer, 0, wxEXPAND | wxALL, 6);

    SetSizer(main_sizer);
}

void ChatPanel::bind_events() noexcept {
    m_send_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_send_action, this);
    m_attach_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_attach_action, this);
    m_copy_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_copy_action, this);
    m_prompt_input_ptr->Bind(wxEVT_KEY_DOWN, &ChatPanel::on_prompt_key_down, this);
}

void ChatPanel::on_attach_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    m_error_banner_ptr->HideAlert();

    wxFileDialog file_dialog(
        this, "Select File Assets for Ingestion", "", "",
        "All Support Formats|*.txt;*.cpp;*.hpp;*.py;*.json;*.log;*.png;*.jpg;*.jpeg",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

    if (file_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    wxArrayString selected_paths;
    file_dialog.GetPaths(selected_paths);

    for (size_t index = 0; index < selected_paths.GetCount(); ++index) {
        std::string target_path = selected_paths[index].ToStdString();
        auto result = m_attachment_engine->AnalyzeAndAdd(target_path);

        if (!result) {
            switch (result.error()) {
                case engine::storage::IngestionError::IMAGE_TOO_LARGE: {
                    m_error_banner_ptr->ShowAlert(
                        "File boundary crossed: Images must be under 4MB to protect VRAM paths.",
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
            }
        }
    }

    refresh_attachment_tray();
}

void ChatPanel::refresh_attachment_tray() noexcept {
    m_tray_sizer_ptr->Clear(true);
    const auto &attachments = m_attachment_engine->GetPendingAttachments();

    if (attachments.empty()) {
        m_tray_container_ptr->Hide();
        Layout();
        return;
    }

    for (const auto &info : attachments) {
        auto *chip = new (std::nothrow) wxPanel(m_tray_container_ptr, wxID_ANY, wxDefaultPosition,
                                                wxDefaultSize, wxBORDER_SIMPLE);
        auto *chip_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);

        std::string prefix =
            (info.m_type == engine::storage::AttachmentType::IMAGE) ? "🖼️ " : "📄 ";
        auto *label = new (std::nothrow)
            wxStaticText(chip, wxID_ANY, wxString::FromUTF8(prefix + info.m_file_name));
        label->SetForegroundColour(wxColour("#F5F5F7"));

        chip_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
        chip->SetSizer(chip_sizer);
        chip->SetBackgroundColour(wxColour("#420912"));

        m_tray_sizer_ptr->Add(chip, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
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

    // Context Injection Loop for Text Documents
    for (const auto &file : m_attachment_engine->GetPendingAttachments()) {
        if (file.m_type == engine::storage::AttachmentType::TEXT_DOCUMENT) {
            auto text_extraction = m_attachment_engine->ExtractTextContent(file);
            if (text_extraction) {
                final_processed_prompt += text_extraction.value();
            }
        }
    }

    append_user_message(raw_prompt.ToStdString());

    wxCommandEvent custom_event(EVT_USER_PROMPT, GetId());
    custom_event.SetString(
        wxString::FromUTF8(final_processed_prompt.data(), final_processed_prompt.size()));
    wxPostEvent(this, custom_event);

    m_attachment_engine->ClearQueue();
    refresh_attachment_tray();
}

void ChatPanel::render_chat_stream() noexcept {
    if (!m_chat_display_ptr) {
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
}

void ChatPanel::append_token(std::string_view token_segment) noexcept {
    m_active_response_stream.append(token_segment.data(), token_segment.size());
    render_chat_stream();
}

void ChatPanel::append_user_message(std::string_view message) noexcept {
    if (!m_active_response_stream.empty()) {
        m_last_llm_response = m_active_response_stream;
        m_raw_markdown_history += "\n" + m_active_response_stream + "\n\n---";
        m_active_response_stream.clear();
    }
    m_raw_markdown_history += "\n\n### 👤 User\n" + std::string(message) + "\n\n### 🤖 malama\n";
    render_chat_stream();
}

void ChatPanel::load_history(const core::ChatSession &session) noexcept {
    m_raw_markdown_history = "### System Channel\n`malama v0.2.7-multimodal` synchronized.\n\n---";
    m_active_response_stream.clear();
    m_last_llm_response.clear();

    for (const auto &message : session.m_messages) {
        if (message.m_role == core::MessageRole::User) {
            m_raw_markdown_history += "\n\n### 👤 User\n" + message.m_content;
        } else if (message.m_role == core::MessageRole::Assistant) {
            m_raw_markdown_history += "\n\n### 🤖 malama\n" + message.m_content + "\n\n---";
            m_last_llm_response = message.m_content;
        } else if (message.m_role == core::MessageRole::System) {
            m_raw_markdown_history += "\n\n### ⚙️ System Context\n" + message.m_content + "\n\n---";
        }
    }
    render_chat_stream();
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
    // Retained from parent reference rules for tracking actions
}

void ChatPanel::start_spinner() noexcept {
    m_spinner_ptr->Show();
    m_spinner_ptr->Start();
    Layout();
}

void ChatPanel::stop_spinner() noexcept {
    m_spinner_ptr->Stop();
    m_spinner_ptr->Hide();
    Layout();
}

}  // namespace malama::ui

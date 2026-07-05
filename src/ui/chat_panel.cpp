// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.cpp
// Purpose:     Implements interactive dialogue views with system toast alerts
// Author:      Magpiny <magpinyb@proton.me>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/chat_panel.hpp"

#include <new>
#include <spdlog/spdlog.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/notifmsg.h>
#include <wx/sizer.h>

#include "common/constants.hpp"
#include "config/config_manager.hpp"
#include "engine/markdown/pipeline.hpp"

namespace malama::ui {

wxDEFINE_EVENT(EVT_USER_PROMPT, wxCommandEvent);
wxDEFINE_EVENT(EVT_CANCEL_GENERATION, wxCommandEvent);

ChatPanel::ChatPanel(wxWindow *parent_ptr) : wxPanel(parent_ptr, wxID_ANY) {
    SetBackgroundColour(wxColour(std::string(constants::color_dark_maroon)));
    setup_layout();
    bind_events();

    auto &config_manager = malama::config::ConfigManager::get_instance();
    config_manager.register_observer([this](const malama::config::AppConfig &config) {
        this->SetBackgroundColour(wxColour(config.m_appearance.m_bg_color));
        if (m_prompt_input_ptr != nullptr) {
            m_prompt_input_ptr->SetBackgroundColour(wxColour(config.m_appearance.m_surface_color));
            m_prompt_input_ptr->SetForegroundColour(wxColour(config.m_appearance.m_text_primary));
        }
        this->Refresh();
    });
}

void ChatPanel::setup_layout() noexcept {
    auto *main_sizer = new (std::nothrow) wxBoxSizer(wxVERTICAL);
    auto *chat_display = new (std::nothrow)
        wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
    m_chat_display_ptr = chat_display;

    m_raw_markdown_history = "### System Status\n`malama v0.2.6` initialized.\n\n---";
    render_chat_stream();

    auto *action_bar_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);

    auto *prompt_input = new (std::nothrow) wxTextCtrl(
        this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, constants::input_area_height_pixels),
        wxTE_MULTILINE | wxTE_RICH2 | wxTE_PROCESS_ENTER | wxBORDER_NONE);
    m_prompt_input_ptr = prompt_input;

    auto *spinner = new (std::nothrow) wxActivityIndicator(this, wxID_ANY);
    m_spinner_ptr = spinner;

    auto *copy_btn = new (std::nothrow)
        wxButton(this, wxID_ANY, wxString::FromUTF8("\U0001F4CB Copy"), wxDefaultPosition,
                 wxDefaultSize, wxBU_EXACTFIT | wxBORDER_NONE);
    m_copy_button_ptr = copy_btn;

    auto *send_btn = new (std::nothrow)
        wxButton(this, wxID_ANY, wxString::FromUTF8("\U0001F4E4 Send"), wxDefaultPosition,
                 wxDefaultSize, wxBU_EXACTFIT | wxBORDER_NONE);
    m_send_button_ptr = send_btn;

    if (main_sizer == nullptr || m_chat_display_ptr == nullptr || action_bar_sizer == nullptr ||
        m_prompt_input_ptr == nullptr || m_spinner_ptr == nullptr || m_copy_button_ptr == nullptr ||
        m_send_button_ptr == nullptr) {
        delete main_sizer;
        delete m_chat_display_ptr;
        delete action_bar_sizer;
        delete m_prompt_input_ptr;
        delete m_spinner_ptr;
        delete m_copy_button_ptr;
        delete m_send_button_ptr;
        spdlog::error("Critical heap depletion inside ChatPanel layout pipeline.");
        return;
    }

    // FIXED: Layout actions attached directly to control bounds to restore rendering paths
    m_spinner_ptr->SetToolTip("Stop prompt generation");
    m_spinner_ptr->SetCursor(wxCursor(wxCURSOR_HAND));

    m_copy_button_ptr->SetToolTip("Copy response to clipboard");
    m_copy_button_ptr->SetCursor(wxCursor(wxCURSOR_HAND));

    m_send_button_ptr->SetToolTip("Send prompt");
    m_send_button_ptr->SetCursor(wxCursor(wxCURSOR_HAND));

    m_spinner_ptr->Hide();

    action_bar_sizer->Add(m_prompt_input_ptr, constants::layout_proportion_stretch,
                          wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin);
    action_bar_sizer->Add(m_spinner_ptr, constants::layout_proportion_fixed,
                          wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin);
    action_bar_sizer->Add(m_copy_button_ptr, constants::layout_proportion_fixed,
                          wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin);
    action_bar_sizer->Add(m_send_button_ptr, constants::layout_proportion_fixed,
                          wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin);

    main_sizer->Add(m_chat_display_ptr, constants::layout_proportion_stretch, wxALL | wxEXPAND,
                    constants::default_margin_padding);
    main_sizer->Add(action_bar_sizer, constants::layout_proportion_fixed,
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, constants::default_margin_padding);

    SetSizer(main_sizer);
}

void ChatPanel::bind_events() noexcept {
    if (m_send_button_ptr != nullptr) {
        m_send_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_send_action, this);
    }
    if (m_copy_button_ptr != nullptr) {
        m_copy_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_copy_action, this);
    }
    if (m_prompt_input_ptr != nullptr) {
        m_prompt_input_ptr->Bind(wxEVT_TEXT_ENTER, &ChatPanel::on_send_action, this);
        m_prompt_input_ptr->Bind(wxEVT_KEY_DOWN, &ChatPanel::on_prompt_key_down, this);
    }
    if (m_chat_display_ptr != nullptr) {
        m_chat_display_ptr->Bind(wxEVT_HTML_LINK_CLICKED, &ChatPanel::on_link_clicked, this);
    }
    if (m_spinner_ptr != nullptr) {
        m_spinner_ptr->Bind(wxEVT_LEFT_DOWN, &ChatPanel::on_spinner_mouse_down, this);
    }
}

auto ChatPanel::render_chat_stream() noexcept -> void {
    if (m_chat_display_ptr == nullptr) {
        return;
    }

    auto theme = malama::config::ConfigManager::get_instance().get_config().m_appearance;
    malama::engine::markdown::Pipeline markdown_engine(theme);

    std::string composite_markdown = m_raw_markdown_history + "\n" + m_active_response_stream;
    std::string html_body = markdown_engine.process(composite_markdown);

    std::string complete_html_document =
        "<html><head><meta charset=\"utf-8\"></head>"
        "<body bgcolor=\"" +
        theme.m_bg_color +
        "\">"
        "<font color=\"" +
        theme.m_text_primary + R"(" face="sans-serif">)" + html_body + "</font></body></html>";

    m_chat_display_ptr->SetPage(
        wxString::FromUTF8(complete_html_document.data(), complete_html_document.size()));

    int coord_x = 0;
    int coord_y = 0;
    m_chat_display_ptr->GetViewStart(&coord_x, &coord_y);
    m_chat_display_ptr->Scroll(coord_x, coord_y + 100);
}

void ChatPanel::append_token(std::string_view token_segment) noexcept {
    m_active_response_stream.append(token_segment.data(), token_segment.size());
    render_chat_stream();
}

auto ChatPanel::append_user_message(std::string_view message) noexcept -> void {
    if (!m_active_response_stream.empty()) {
        m_last_llm_response = m_active_response_stream;
        m_raw_markdown_history.push_back('\n');
        m_raw_markdown_history += m_active_response_stream;
        m_raw_markdown_history += "\n\n---";
        m_active_response_stream.clear();
    }

    m_raw_markdown_history += "\n\n### \U0001F464 User\n";
    m_raw_markdown_history += std::string(message);
    m_raw_markdown_history += "\n\n### \U0001F916 malama\n";
    render_chat_stream();
}

void ChatPanel::on_send_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    if (m_prompt_input_ptr == nullptr) {
        return;
    }

    const auto prompt_text = m_prompt_input_ptr->GetValue();
    if (prompt_text.IsEmpty()) {
        return;
    }

    m_prompt_input_ptr->Clear();

    wxCommandEvent custom_event(EVT_USER_PROMPT, GetId());
    custom_event.SetString(prompt_text);
    custom_event.SetEventObject(this);

    wxPostEvent(this, custom_event);
}

void ChatPanel::on_copy_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    if (wxTheClipboard->Open()) {
        std::string text_to_copy =
            m_active_response_stream.empty() ? m_last_llm_response : m_active_response_stream;

        auto *data_obj = new (std::nothrow)
            wxTextDataObject(wxString::FromUTF8(text_to_copy.data(), text_to_copy.size()));
        if (data_obj != nullptr) {
            wxTheClipboard->SetData(data_obj);
            wxNotificationMessage notification("malama", "Response copied!");
            notification.Show(wxICON_INFORMATION);
        }
        wxTheClipboard->Close();
    }
}

void ChatPanel::on_link_clicked(wxHtmlLinkEvent &event) noexcept {
    wxString href = event.GetLinkInfo().GetHref();

    if (href.StartsWith("malama://copy_code:")) {
        wxString hex_part = href.Mid(19);
        std::string raw_code;
        std::string std_hex = hex_part.ToStdString();
        raw_code.reserve(std_hex.size() / 2);
        for (size_t index = 0; index + 1 < std_hex.size(); index += 2) {
            char high_nibble = std_hex[index];
            char low_nibble = std_hex[index + 1];
            int high_val = (high_nibble >= 'A') ? (high_nibble - 'A' + 10) : (high_nibble - '0');
            int low_val = (low_nibble >= 'A') ? (low_nibble - 'A' + 10) : (low_nibble - '0');
            raw_code.push_back(static_cast<char>((high_val << 4) | low_val));
        }
        if (wxTheClipboard->Open()) {
            auto *data_obj = new (std::nothrow)
                wxTextDataObject(wxString::FromUTF8(raw_code.data(), raw_code.size()));
            if (data_obj != nullptr) {
                wxTheClipboard->SetData(data_obj);
                wxNotificationMessage notification("malama", "Code copied!");
                notification.Show(wxICON_INFORMATION);
            }
            wxTheClipboard->Close();
        }
    } else if (href.StartsWith("malama://download_code:")) {
        wxString hex_part = href.Mid(23);
        std::string raw_code;
        std::string std_hex = hex_part.ToStdString();
        raw_code.reserve(std_hex.size() / 2);
        for (size_t index = 0; index + 1 < std_hex.size(); index += 2) {
            char high_nibble = std_hex[index];
            char low_nibble = std_hex[index + 1];
            int high_val = (high_nibble >= 'A') ? (high_nibble - 'A' + 10) : (high_nibble - '0');
            int low_val = (low_nibble >= 'A') ? (low_nibble - 'A' + 10) : (low_nibble - '0');
            raw_code.push_back(static_cast<char>((high_val << 4) | low_val));
        }
        wxFileDialog saveFileDialog(this, "Save code block", "", "", "All files (*.*)|*.*",
                                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (saveFileDialog.ShowModal() == wxID_OK) {
            wxString path = saveFileDialog.GetPath();
            wxFile file(path, wxFile::write);
            if (file.IsOpened()) {
                file.Write(raw_code.data(), raw_code.size());
                file.Close();
                wxNotificationMessage notification("malama", "Code downloaded!");
                notification.Show(wxICON_INFORMATION);
            }
        }
    }
}

void ChatPanel::on_prompt_key_down(wxKeyEvent &event) noexcept {
    const auto key_code = event.GetKeyCode();
    if (key_code == WXK_RETURN || key_code == WXK_NUMPAD_ENTER) {
        if (event.ShiftDown()) {
            if (m_prompt_input_ptr != nullptr) {
                m_prompt_input_ptr->WriteText("\n");
            }
        } else {
            wxCommandEvent dummy_evt;
            on_send_action(dummy_evt);
        }
    } else {
        event.Skip();
    }
}

void ChatPanel::on_spinner_mouse_down(wxMouseEvent &WXUNUSED(event)) noexcept {
    stop_spinner();
    wxCommandEvent cancel_event(EVT_CANCEL_GENERATION, GetId());
    cancel_event.SetEventObject(this);
    wxPostEvent(this, cancel_event);
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

void ChatPanel::load_history(const core::ChatSession &session) noexcept {
    m_raw_markdown_history = "### System Status\n`malama v0.2.6-alpha` initialized.\n\n---";
    m_active_response_stream.clear();
    m_last_llm_response.clear();

    for (const auto &message : session.m_messages) {
        if (message.m_role == core::MessageRole::User) {
            m_raw_markdown_history += "\n\n### \U0001F464 User\n";
            m_raw_markdown_history += message.m_content;
        } else if (message.m_role == core::MessageRole::Assistant) {
            m_raw_markdown_history += "\n\n### \U0001F916 malama\n";
            m_raw_markdown_history += message.m_content;
            m_raw_markdown_history += "\n\n---";
            m_last_llm_response = message.m_content;
        } else if (message.m_role == core::MessageRole::System) {
            m_raw_markdown_history += "\n\n### \u2699\ufe0f System Context\n";
            m_raw_markdown_history += message.m_content;
            m_raw_markdown_history += "\n\n---";
        }
    }
    render_chat_stream();
}

auto ChatPanel::get_active_response_stream() const noexcept -> std::string {
    return m_active_response_stream;
}

}  // namespace malama::ui

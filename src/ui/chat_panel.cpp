// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.cpp
// Purpose:     Implements interactive dialogue views with system toast alerts
// Author:      Magpiny <magpinyb@proton.me>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "ui/chat_panel.hpp"
#include "common/constants.hpp"
#include "engine/markdown/pipeline.hpp"

#include <new>
#include <wx/sizer.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/notifmsg.h>
#include <wx/file.h>
#include <wx/filedlg.h>

namespace malama::ui {

wxDEFINE_EVENT(EVT_USER_PROMPT, wxCommandEvent);

/**
 * @brief Initializes the chat panel and registers theme updates.
 *
 * @param parent_ptr Parent window for the panel.
 */
ChatPanel::ChatPanel(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY) {
    SetBackgroundColour(wxColour(std::string(constants::color_dark_maroon)));
    setup_layout();
    bind_events();

    auto& confmgr = malama::config::ConfigManager::get_instance();
    confmgr.register_observer([this](const malama::config::AppConfig& conf) {
        this->SetBackgroundColour(wxColour(conf.m_appearance.m_bg_color));
        if (m_prompt_input_ptr != nullptr) {
            m_prompt_input_ptr->SetBackgroundColour(
                wxColour(conf.m_appearance.m_surface_color)
            );
            m_prompt_input_ptr->SetForegroundColour(
                wxColour(conf.m_appearance.m_text_primary)
            );
        }
        this->Refresh();
    });
}

/**
 * @brief Creates and arranges the chat panel controls.
 *
 * Initializes the chat display, prompt input, and action buttons, then sets the panel sizer.
 */
void ChatPanel::setup_layout() noexcept {
    auto *main_sizer = new (std::nothrow) wxBoxSizer(wxVERTICAL);
    
    m_chat_display_ptr = new (std::nothrow) wxHtmlWindow(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO
    );
    
    m_raw_markdown_history = "### System Status\n`malama v0.1.1` initialized.\n\n---";
    render_chat_stream();

    auto *action_bar_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);

    m_prompt_input_ptr = new (std::nothrow) wxTextCtrl(
        this, wxID_ANY, "", wxDefaultPosition, 
        wxSize(-1, constants::input_area_height_pixels), 
        wxTE_MULTILINE | wxTE_RICH2 | wxTE_PROCESS_ENTER | wxBORDER_NONE
    );

    m_copy_button_ptr = new (std::nothrow) wxButton(
        this, wxID_ANY, L"\U0001F4CB Copy", wxDefaultPosition, wxDefaultSize, 
        wxBU_EXACTFIT | wxBORDER_NONE
    );

    m_send_button_ptr = new (std::nothrow) wxButton(
        this, wxID_ANY, L"\U0001F4E4 Send", wxDefaultPosition, wxDefaultSize, 
        wxBU_EXACTFIT | wxBORDER_NONE
    );

    // Arranged layout sequence: Input bar (Left side stretch) -> Copy -> Send
    action_bar_sizer->Add(
        m_prompt_input_ptr, constants::layout_proportion_stretch, 
        wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin
    );
    action_bar_sizer->Add(
        m_copy_button_ptr, constants::layout_proportion_fixed, 
        wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin
    );
    action_bar_sizer->Add(
        m_send_button_ptr, constants::layout_proportion_fixed, 
        wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin
    );

    main_sizer->Add(
        m_chat_display_ptr, constants::layout_proportion_stretch, 
        wxALL | wxEXPAND, constants::default_margin_padding
    );
    main_sizer->Add(
        action_bar_sizer, constants::layout_proportion_fixed, 
        wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, constants::default_margin_padding
    );

    SetSizer(main_sizer);
}

/**
 * @brief Binds the chat panel's UI events to their handlers.
 */
void ChatPanel::bind_events() noexcept {
    if (m_send_button_ptr != nullptr) {
        m_send_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_send_action, this);
    }
    if (m_copy_button_ptr != nullptr) {
        m_copy_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_copy_action, this);
    }
    if (m_prompt_input_ptr != nullptr) {
        m_prompt_input_ptr->Bind(wxEVT_TEXT_ENTER, &ChatPanel::on_send_action, this);
    }
    if (m_chat_display_ptr != nullptr) {
        m_chat_display_ptr->Bind(wxEVT_HTML_LINK_CLICKED, &ChatPanel::on_link_clicked, this);
    }
}

/**
 * @brief Renders the chat history and active response stream.
 *
 * Updates the HTML chat display using the current theme colors and scrolls the view to show the latest content.
 */
auto ChatPanel::render_chat_stream() noexcept -> void {
    if (m_chat_display_ptr == nullptr) { 
        return; 
    }

    auto theme = malama::config::ConfigManager::get_instance().get_config().m_appearance;
    malama::engine::markdown::Pipeline md_engine(theme);

    std::string composite_markdown = m_raw_markdown_history + "\n" + m_active_response_stream;
    std::string html_body = md_engine.process(composite_markdown);

    std::string complete_html_document = 
        "<html><body bgcolor=\"" + theme.m_bg_color + "\">"
        "<font color=\"" + theme.m_text_primary + R"(" face="sans-serif">)"
        + html_body + 
        "</font></body></html>";

    m_chat_display_ptr->SetPage(
        wxString::FromUTF8(complete_html_document.data(), complete_html_document.size())
    );
    
    int x = 0;
    int y = 0;
    m_chat_display_ptr->GetViewStart(&x, &y);
    m_chat_display_ptr->Scroll(x, y + 100);
}

auto ChatPanel::append_token(std::string_view token_segment) noexcept -> void {
    m_active_response_stream.append(token_segment.data(), token_segment.size());
    render_chat_stream();
}

/**
 * @brief Appends a user message to the chat history.
 *
 * If an assistant response is currently streaming, it is preserved as the last completed response before the new
 * user turn is added.
 *
 * @param message Message text to add.
 */
auto ChatPanel::append_user_message(std::string_view message) noexcept -> void {
    if (!m_active_response_stream.empty()) {
        m_last_llm_response = m_active_response_stream; 
        m_raw_markdown_history += "\n" + m_active_response_stream + "\n\n---";
        m_active_response_stream.clear();
    }

    m_raw_markdown_history += "\n\n### \U0001F464 User\n";
    m_raw_markdown_history += std::string(message);
    m_raw_markdown_history += "\n\n### \U0001F916 malama\n";
    render_chat_stream();
}

/**
 * @brief Posts the current prompt as a user-submitted message.
 *
 * Clears the prompt input and emits an @c EVT_USER_PROMPT event containing the entered text.
 */
void ChatPanel::on_send_action([[maybe_unused]] wxCommandEvent &event) noexcept {
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

/**
 * @brief Copies the current assistant response to the clipboard.
 *
 * Copies the in-progress streamed response when available, otherwise copies the most recent completed response.
 */
void ChatPanel::on_copy_action([[maybe_unused]] wxCommandEvent &event) noexcept {
    if (wxTheClipboard->Open()) {
        std::string text_to_copy = m_active_response_stream.empty() 
                                 ? m_last_llm_response 
                                 : m_active_response_stream;
        
        auto *clipboard_data_ptr = new (std::nothrow) wxTextDataObject(
            wxString::FromUTF8(text_to_copy.data(), text_to_copy.size())
        );
        if (clipboard_data_ptr != nullptr) {
            wxTheClipboard->SetData(clipboard_data_ptr);
            wxNotificationMessage notification("malama", "Response copied!");
            notification.Show(wxICON_INFORMATION);
        }
        wxTheClipboard->Close();
    }
}

/**
 * @brief Handles special code-copy and code-download links.
 *
 * @param event HTML link click event containing the clicked URL.
 */
void ChatPanel::on_link_clicked(wxHtmlLinkEvent &event) noexcept {
    wxString href = event.GetLinkInfo().GetHref();
    
    if (href.StartsWith("malama://copy_code:")) {
        wxString hex_part = href.Mid(19);
        std::string raw_code;
        std::string std_hex = hex_part.ToStdString();
        raw_code.reserve(std_hex.size() / 2);
        for (size_t i = 0; i + 1 < std_hex.size(); i += 2) {
            char high = std_hex[i];
            char low = std_hex[i + 1];
            int h = (high >= 'A') ? (high - 'A' + 10) : (high - '0');
            int l = (low >= 'A') ? (low - 'A' + 10) : (low - '0');
            raw_code.push_back(static_cast<char>((h << 4) | l));
        }
        if (wxTheClipboard->Open()) {
            auto *clipboard_data_ptr = new (std::nothrow) wxTextDataObject(
                wxString::FromUTF8(raw_code.data(), raw_code.size())
            );
            if (clipboard_data_ptr != nullptr) {
                wxTheClipboard->SetData(clipboard_data_ptr);
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
        for (size_t i = 0; i + 1 < std_hex.size(); i += 2) {
            char high = std_hex[i];
            char low = std_hex[i + 1];
            int h = (high >= 'A') ? (high - 'A' + 10) : (high - '0');
            int l = (low >= 'A') ? (low - 'A' + 10) : (low - '0');
            raw_code.push_back(static_cast<char>((h << 4) | l));
        }
        wxFileDialog saveFileDialog(this, "Save code block", "", "",
            "All files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
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

/**
 * @brief Loads a chat session into the conversation view.
 *
 * Clears the current display and replays the session messages into the chat
 * history.
 *
 * @param session Chat session whose messages are rendered.
 */
void ChatPanel::load_history(const core::ChatSession& session) noexcept {
    if (m_html_window_ptr != nullptr) {
        m_html_window_ptr->SetPage(""); 
    }
    
    for (const auto& msg : session.m_messages) {
        if (msg.m_role == core::MessageRole::User) {
            append_user_message(msg.m_content);
        } else {
            append_token(msg.m_content); 
        }
    }
}

} // namespace malama::ui

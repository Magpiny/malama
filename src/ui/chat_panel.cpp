// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.cpp
// Purpose:     Implements the interactive dialogue workspace panel
// Author:      Magpiny <magpinyb@proton.me>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#include "ui/chat_panel.hpp"
#include "common/constants.hpp"
#include "engine/markdown/pipeline.hpp"

#include <new>
#include <wx/sizer.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/notifmsg.h>

namespace malama::ui {

wxDEFINE_EVENT(EVT_USER_PROMPT, wxCommandEvent);

ChatPanel::ChatPanel(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY) {
    SetBackgroundColour(wxColour(std::string(constants::color_dark_maroon)));
    setup_layout();
    bind_events();

    auto& confmgr = malama::config::ConfigManager::get_instance();
    confmgr.register_observer([this](const malama::config::AppConfig& conf) {
        this->SetBackgroundColour(wxColour(conf.m_appearance.m_bg_color));
        if (m_prompt_input_ptr != nullptr) {
            m_prompt_input_ptr->SetBackgroundColour(wxColour(conf.m_appearance.m_surface_color));
            m_prompt_input_ptr->SetForegroundColour(wxColour(conf.m_appearance.m_text_primary));
        }
        this->Refresh();
    });
}

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

    m_send_button_ptr = new (std::nothrow) wxButton(this, wxID_ANY, L"\U0001F4E4 Send", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT | wxBORDER_NONE);

    // Removed the global copy button and layout logic for it
    action_bar_sizer->Add(m_prompt_input_ptr, constants::layout_proportion_stretch, wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin);
    action_bar_sizer->Add(m_send_button_ptr, constants::layout_proportion_fixed, wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin);

    main_sizer->Add(m_chat_display_ptr, constants::layout_proportion_stretch, wxALL | wxEXPAND, constants::default_margin_padding);
    main_sizer->Add(action_bar_sizer, constants::layout_proportion_fixed, wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, constants::default_margin_padding);

    SetSizer(main_sizer);
}

void ChatPanel::bind_events() noexcept {
    if (m_send_button_ptr != nullptr) {
        m_send_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::on_send_action, this);
    }
    if (m_prompt_input_ptr != nullptr) {
        m_prompt_input_ptr->Bind(wxEVT_TEXT_ENTER, &ChatPanel::on_send_action, this);
    }
    if (m_chat_display_ptr != nullptr) {
        // Intercept inline HTML link clicks
        m_chat_display_ptr->Bind(wxEVT_HTML_LINK_CLICKED, &ChatPanel::on_link_clicked, this);
    }
}

auto ChatPanel::render_chat_stream() noexcept -> void {
    if (m_chat_display_ptr == nullptr) { return;
}

    auto theme = malama::config::ConfigManager::get_instance().get_config().m_appearance;
    malama::engine::markdown::Pipeline md_engine(theme);

    std::string composite_markdown = m_raw_markdown_history + "\n" + m_active_response_stream;
    std::string html_body = md_engine.process(composite_markdown);

    // NEW: Inject the clickable copy link at the very bottom of the response stream
    html_body += R"(<br><div align="right"><b><a href="malama://copy_last"><font color=")" + theme.m_text_accent + "\">[ \U0001F4CB Copy Latest Response ]</font></a></b></div><br>";

    std::string complete_html_document = 
        "<html><body bgcolor=\"" + theme.m_bg_color + "\">"
        "<font color=\"" + theme.m_text_primary + R"(" face="sans-serif">)"
        + html_body + 
        "</font></body></html>";

    m_chat_display_ptr->SetPage(wxString::FromUTF8(complete_html_document.data(), complete_html_document.size()));
    
    int x = 0, y = 0;
    m_chat_display_ptr->GetViewStart(&x, &y);
    m_chat_display_ptr->Scroll(x, y + 100);
}

auto ChatPanel::append_token(std::string_view token_segment) noexcept -> void {
    m_active_response_stream.append(token_segment.data(), token_segment.size());
    render_chat_stream();
}

auto ChatPanel::append_user_message(std::string_view message) noexcept -> void {
    if (!m_active_response_stream.empty()) {
        m_last_llm_response = m_active_response_stream; // Cache latest response isolated
        m_raw_markdown_history += "\n" + m_active_response_stream + "\n\n---";
        m_active_response_stream.clear();
    }

    m_raw_markdown_history += "\n\n### \U0001F464 User\n" + std::string(message) + "\n\n### \U0001F916 malama\n";
    render_chat_stream();
}

void ChatPanel::on_send_action([[maybe_unused]] wxCommandEvent &event) noexcept {
    if (m_prompt_input_ptr == nullptr) { return;
}

    const auto prompt_text = m_prompt_input_ptr->GetValue();
    if (prompt_text.IsEmpty()) { return;
}

    m_prompt_input_ptr->Clear();

    wxCommandEvent custom_event(EVT_USER_PROMPT, GetId());
    custom_event.SetString(prompt_text);
    custom_event.SetEventObject(this);
    
    wxPostEvent(this, custom_event);
}

void ChatPanel::on_link_clicked(wxHtmlLinkEvent &event) noexcept {
    wxString href = event.GetLinkInfo().GetHref();
    
    if (href == "malama://copy_last") {
        if (wxTheClipboard->Open()) {
            // Copy active generation if streaming, otherwise copy the last completed block
            std::string text_to_copy = m_active_response_stream.empty() ? m_last_llm_response : m_active_response_stream;
            
            auto *clipboard_data_ptr = new (std::nothrow) wxTextDataObject(wxString::FromUTF8(text_to_copy.data(), text_to_copy.size()));
            if (clipboard_data_ptr != nullptr) {
                wxTheClipboard->SetData(clipboard_data_ptr);
                wxNotificationMessage notification("malama", "Response copied!");
                notification.Show(wxICON_INFORMATION);
            }
            wxTheClipboard->Close();
        }
    }
}

} // namespace malama::ui

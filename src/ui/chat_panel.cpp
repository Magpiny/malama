// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.cpp
// Purpose:     Implements the interactive dialogue workspace panel
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "ui/chat_panel.hpp"
#include "common/constants.hpp"

#include <new>

#include <wx/sizer.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/notifmsg.h>

namespace malama::ui {

wxDEFINE_EVENT(EVT_USER_PROMPT, wxCommandEvent);

ChatPanel::ChatPanel(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY) {
    SetBackgroundColour(wxColour(std::string(constants::color_earth_brown)));
    SetupLayout();
    BindEvents();
}

void ChatPanel::SetupLayout() noexcept {
    wxBoxSizer *main_sizer_ptr = new (std::nothrow) wxBoxSizer(wxVERTICAL); // NOLINT
    if (main_sizer_ptr == nullptr) {return;}

   m_chat_display_ptr = new (std::nothrow) wxTextCtrl(
        this, wxID_ANY, "malama v0.1.0 MVP Initialized.\nSystem connected and ready.\n",
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE
    );
    if (m_chat_display_ptr == nullptr) { delete main_sizer_ptr; return; } // NOLINT
    m_chat_display_ptr->SetBackgroundColour(wxColour(std::string(constants::color_earth_brown)));
    m_chat_display_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    wxBoxSizer *action_bar_sizer_ptr = new (std::nothrow) wxBoxSizer(wxHORIZONTAL); // NOLINT
    if (action_bar_sizer_ptr == nullptr) { delete main_sizer_ptr; return; } // NOLINT

   m_prompt_input_ptr = new (std::nothrow) wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER | wxBORDER_NONE);
    m_send_button_ptr = new (std::nothrow) wxButton(this, wxID_ANY, L"\U0001F4E4 Send", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT | wxBORDER_NONE);
   m_copy_button_ptr = new (std::nothrow) wxButton(this, wxID_ANY, L"\U0001F4CB Copy", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT | wxBORDER_NONE);

    if (m_prompt_input_ptr == nullptr || m_send_button_ptr == nullptr || m_copy_button_ptr == nullptr) {return;}

    m_prompt_input_ptr->SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    m_prompt_input_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    m_send_button_ptr->SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    m_send_button_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    m_copy_button_ptr->SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    m_copy_button_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));
    
    // Fixed: Tooltip implementation for hover interactions
    m_copy_button_ptr->SetToolTip("Copy entire conversation to system clipboard");

    // Layout Engine: Giving the input bar 'stretch' proportion eats all left-side space,
    // pushing Send and Copy buttons tightly into the bottom right corner.
    action_bar_sizer_ptr->Add(m_prompt_input_ptr, constants::layout_proportion_stretch, wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin);
    action_bar_sizer_ptr->Add(m_send_button_ptr, constants::layout_proportion_fixed, wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin);
    action_bar_sizer_ptr->Add(m_copy_button_ptr, constants::layout_proportion_fixed, wxALIGN_CENTER_VERTICAL | wxALL, constants::icon_button_margin);

    main_sizer_ptr->Add(m_chat_display_ptr, constants::layout_proportion_stretch, wxALL | wxEXPAND, constants::default_margin_padding);
    main_sizer_ptr->Add(action_bar_sizer_ptr, constants::layout_proportion_fixed, wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, constants::default_margin_padding);

    SetSizer(main_sizer_ptr);
}

void ChatPanel::BindEvents() noexcept {
    if (m_copy_button_ptr != nullptr) {
        m_copy_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::OnCopyAction, this);
    }
    if (m_send_button_ptr != nullptr) {
        m_send_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::OnSendAction, this);
    }
    if (m_prompt_input_ptr != nullptr) {
        m_prompt_input_ptr->Bind(wxEVT_TEXT_ENTER, &ChatPanel::OnSendAction, this);
    }
}

auto ChatPanel::AppendToken(std::string_view token_segment) noexcept -> void {
    if (m_chat_display_ptr == nullptr) [[unlikely]] {return;}
    m_chat_display_ptr->AppendText(wxString::FromUTF8(token_segment.data(), token_segment.size()));
    m_chat_display_ptr->SetInsertionPointEnd();
}

auto ChatPanel::AppendUserMessage(std::string_view message) noexcept -> void {
    if (m_chat_display_ptr == nullptr) [[unlikely]] {return;}

    // CRITICAL FIX: Bypass the variadic Format() segfault by using safe wxString stream operations
    wxString safe_formatted_text;
    safe_formatted_text << "\n\n\U0001F464 You:\n" 
                        << wxString::FromUTF8(message.data(), message.size())
                        << "\n\n\U0001F916 malama:\n";

    m_chat_display_ptr->AppendText(safe_formatted_text);
    m_chat_display_ptr->SetInsertionPointEnd();
}

void ChatPanel::OnCopyAction([[maybe_unused]] wxCommandEvent &event) noexcept {
    if (m_chat_display_ptr == nullptr) {return;}
    
    if (wxTheClipboard->Open()) {
        auto *clipboard_data_ptr = new (std::nothrow) wxTextDataObject(m_chat_display_ptr->GetValue()); // NOLINT
        if (clipboard_data_ptr != nullptr) {
            wxTheClipboard->SetData(clipboard_data_ptr);
            
            // Fixed: Trigger native Linux OS Notification daemon
            wxNotificationMessage notification("malama", "Copied to clipboard!");
            notification.Show(wxICON_INFORMATION);
        }
        wxTheClipboard->Close();
    }
}

void ChatPanel::OnSendAction([[maybe_unused]] wxCommandEvent &event) noexcept {
    if (m_prompt_input_ptr == nullptr) {return;}

    const auto prompt_text = m_prompt_input_ptr->GetValue();
    if (prompt_text.IsEmpty()) {return;}

    m_prompt_input_ptr->Clear();

    wxCommandEvent custom_event(EVT_USER_PROMPT, GetId());
    custom_event.SetString(prompt_text);
    custom_event.SetEventObject(this);
    
    // Fixed: Use wxPostEvent to safely queue the UI interaction rather than process it synchronously
    wxPostEvent(this, custom_event);
}

} // namespace malama::ui

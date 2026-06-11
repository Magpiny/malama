// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.cpp
// Purpose:     Implements the interactive dialogue workspace panel
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
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

namespace malama::ui {

ChatPanel::ChatPanel(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY) {
    
    // Apply UI Theming
    SetBackgroundColour(wxColour(std::string(constants::color_earth_brown)));

    SetupLayout();
    BindEvents();
}

void ChatPanel::SetupLayout() noexcept {
    wxBoxSizer *main_sizer_ptr = new (std::nothrow) wxBoxSizer(wxVERTICAL); // NOLINT
    if (main_sizer_ptr == nullptr) {
        return;
    }

    m_chat_display_ptr = new (std::nothrow) wxTextCtrl(
        this, wxID_ANY, "Conversation workspace initialized. Standing by for token stream...\n",
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE
    );
    if (m_chat_display_ptr == nullptr) {
        delete main_sizer_ptr; // NOLINT
        return;
    }
    
    m_chat_display_ptr->SetBackgroundColour(wxColour(std::string(constants::color_earth_brown)));
    m_chat_display_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    // Create a horizontal sizer for the bottom action bar
    wxBoxSizer *action_bar_sizer_ptr = new (std::nothrow) wxBoxSizer(wxHORIZONTAL); // NOLINT
    if (action_bar_sizer_ptr == nullptr) {
        delete main_sizer_ptr; // NOLINT
        return;
    }

    // Provision a minimalist button requesting native exact-fit rounding from the GTK compositor
    m_copy_button_ptr = new (std::nothrow) wxButton(
        this, wxID_ANY, L"\U0001F4CB Copy", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT | wxBORDER_NONE
    );
    if (m_copy_button_ptr == nullptr) {
        delete action_bar_sizer_ptr; // NOLINT
        delete main_sizer_ptr;       // NOLINT
        return;
    }
    
    m_copy_button_ptr->SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    m_copy_button_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    // Assemble the structural hierarchy
    action_bar_sizer_ptr->Add(m_copy_button_ptr, constants::layout_proportion_fixed, wxALIGN_LEFT | wxALL, constants::icon_button_margin);

    main_sizer_ptr->Add(m_chat_display_ptr, constants::layout_proportion_stretch, wxALL | wxEXPAND, constants::default_margin_padding);
    main_sizer_ptr->Add(action_bar_sizer_ptr, constants::layout_proportion_fixed, wxEXPAND | wxLEFT | wxBOTTOM, constants::default_margin_padding);

    SetSizer(main_sizer_ptr);
}

void ChatPanel::BindEvents() noexcept {
    if (m_copy_button_ptr != nullptr) {
        m_copy_button_ptr->Bind(wxEVT_BUTTON, &ChatPanel::OnCopyAction, this);
    }
}

auto ChatPanel::AppendToken(std::string_view token_segment) noexcept -> void {
    if (m_chat_display_ptr == nullptr) [[unlikely]] {
        return;
    }
    m_chat_display_ptr->AppendText(wxString::FromUTF8(token_segment.data(), token_segment.size()));
    
    // Enforce infinite terminal scrolling lock to the latest token frame
    m_chat_display_ptr->SetInsertionPointEnd();
}

void ChatPanel::OnCopyAction([[maybe_unused]] wxCommandEvent &event) noexcept {
    if (m_chat_display_ptr == nullptr) {
        return;
    }

    // Access the OS-level clipboard pipeline securely
    if (wxTheClipboard->Open()) {
        auto *clipboard_data_ptr = new (std::nothrow) wxTextDataObject(m_chat_display_ptr->GetValue()); // NOLINT
        if (clipboard_data_ptr != nullptr) {
            // SetData returns true on success and takes ownership; if it fails, we must delete
            if (!wxTheClipboard->SetData(clipboard_data_ptr)) {
                delete clipboard_data_ptr; // NOLINT
            }
        }
        wxTheClipboard->Close();
    }
}

} // namespace malama::ui

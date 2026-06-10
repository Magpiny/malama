// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/sidebar_panel.cpp
// Purpose:     Implements the structural sidebar control mechanics
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "ui/sidebar_panel.hpp"
#include "common/constants.hpp"

#include <new>

#include <wx/sizer.h>
#include <wx/stattext.h>

namespace malama::ui {

SidebarPanel::SidebarPanel(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY) {
    
    // Apply UI Theming
    SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    
    SetupLayout();
}

void SidebarPanel::SetupLayout() noexcept {
    wxBoxSizer *sizer_ptr = new (std::nothrow) wxBoxSizer(wxVERTICAL); // NOLINT
    if (sizer_ptr == nullptr) {
        return;
    }

    auto *header_ptr = new (std::nothrow) wxStaticText(this, wxID_ANY, "Historical Sessions");
    if (header_ptr == nullptr) {
        delete sizer_ptr; // NOLINT
        return;
    }
    header_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    m_history_list_ptr = new (std::nothrow) wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxBORDER_NONE);
    if (m_history_list_ptr == nullptr) {
        delete header_ptr; // NOLINT
        delete sizer_ptr;  // NOLINT
        return;
    }
    m_history_list_ptr->SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    m_history_list_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    sizer_ptr->Add(header_ptr, constants::layout_proportion_fixed, wxALL | wxEXPAND, constants::default_margin_padding);
    sizer_ptr->Add(m_history_list_ptr, constants::layout_proportion_stretch, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, constants::default_margin_padding);

    SetSizer(sizer_ptr);
}

} // namespace malama::ui

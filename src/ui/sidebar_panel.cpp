// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/sidebar_panel.cpp
// Purpose:     Implements the structural sidebar control mechanics
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-08
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "ui/sidebar_panel.hpp"
#include "common/constants.hpp"

#include <new>

#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace malama::ui {

SidebarPanel::SidebarPanel(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY) {
    SetupLayout();
}

<<<<<<< HEAD
void SidebarPanel::SetupLayout() noexcept {    
    auto *sizer_ptr = new (std::nothrow) wxBoxSizer(wxVERTICAL); // NOLINT(cppcoreguidelines-owning-memory)
=======
void SidebarPanel::SetupLayout() noexcept {
    auto *sizer_ptr = new (std::nothrow) wxBoxSizer(wxVERTICAL);
>>>>>>> 9de69a21ca5de4c99f740cd8bd28fe7ff675acb1
    if (sizer_ptr == nullptr) {
        return;
    }

    auto *header_ptr = new (std::nothrow) wxStaticText(this, wxID_ANY, "Historical Sessions");
    if (header_ptr == nullptr) {
        delete sizer_ptr; // NOLINT(cppcoreguidelines-owning-memory)
        return;
    }

    m_history_list_ptr = new (std::nothrow) wxListBox(this, wxID_ANY);
    if (m_history_list_ptr == nullptr) {
        delete header_ptr; // NOLINT(cppcoreguidelines-owning-memory)
        delete sizer_ptr;  // NOLINT(cppcoreguidelines-owning-memory)
        return;
    }

    // Stack the elements vertically with unified margins
    sizer_ptr->Add(header_ptr, 0, wxALL | wxEXPAND, constants::default_margin_padding);
    sizer_ptr->Add(m_history_list_ptr, 1, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, constants::default_margin_padding);

    SetSizer(sizer_ptr);
}

} // namespace malama::ui

// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/main_frame.cpp
// Purpose:     Implements top-level window controls and menu modal dialog loops
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-08
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "ui/main_frame.hpp"
#include "ui/sidebar_panel.hpp"
#include "ui/chat_panel.hpp"
#include "common/constants.hpp"

#include <new>
#include <string>

#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <spdlog/spdlog.h>

namespace malama::ui {

wxDEFINE_EVENT(EVT_MALAMA_TOKEN, wxThreadEvent);

MainFrame::MainFrame(const wxString &title, const wxPoint &pos, const wxSize &size)
    : wxFrame(nullptr, wxID_ANY, title, pos, size) {
    SetupMenuBar();
    SetupWorkspaceLayout();
    BindActionEvents();
}

void MainFrame::SetupMenuBar() noexcept {
    auto *menu_bar_ptr = new (std::nothrow) wxMenuBar(); // NOLINT(cppcoreguidelines-owning-memory)
    if (menu_bar_ptr == nullptr) {
        return;
    }

    auto *file_menu_ptr = new (std::nothrow) wxMenu(); // NOLINT(cppcoreguidelines-owning-memory)
    if (file_menu_ptr == nullptr) {
        delete menu_bar_ptr; // NOLINT(cppcoreguidelines-owning-memory)
        return;
    }
    file_menu_ptr->Append(static_cast<int>(MenuId::ExitId), "E&xit\tAlt-X", "Terminate application framework");
    menu_bar_ptr->Append(file_menu_ptr, "&File");

    auto *help_menu_ptr = new (std::nothrow) wxMenu(); // NOLINT(cppcoreguidelines-owning-memory)
    if (help_menu_ptr == nullptr) {
        delete menu_bar_ptr; // NOLINT(cppcoreguidelines-owning-memory)
        return;
    }
    help_menu_ptr->Append(static_cast<int>(MenuId::LicenceId), "&Licence", "Display open-source licensing constraints");
    help_menu_ptr->Append(static_cast<int>(MenuId::AboutId), "&About...", "Display platform implementation details");
    menu_bar_ptr->Append(help_menu_ptr, "&Help");

    SetMenuBar(menu_bar_ptr);
}

void MainFrame::SetupWorkspaceLayout() noexcept {
    // Fixed: Added NOLINT check for framework sizer allocations
    auto *main_sizer_ptr = new (std::nothrow) wxBoxSizer(wxHORIZONTAL); // NOLINT(cppcoreguidelines-owning-memory)
    if (main_sizer_ptr == nullptr) {
        return;
    }

    m_sidebar_panel_ptr = new (std::nothrow) SidebarPanel(this);
    if (m_sidebar_panel_ptr == nullptr) {
        delete main_sizer_ptr; // NOLINT(cppcoreguidelines-owning-memory)
        return;
    }

    m_chat_panel_ptr = new (std::nothrow) ChatPanel(this);
    if (m_chat_panel_ptr == nullptr) {
        delete m_sidebar_panel_ptr; // NOLINT(cppcoreguidelines-owning-memory)
        delete main_sizer_ptr;      // NOLINT(cppcoreguidelines-owning-memory)
        return;
    }

    // Fixed: Substituted layout integers with explicit structural weight parameters
    main_sizer_ptr->Add(m_sidebar_panel_ptr, constants::sidebar_layout_weight, wxEXPAND | wxALL, constants::zero_margin_padding);
    main_sizer_ptr->Add(m_chat_panel_ptr, constants::chat_layout_weight, wxEXPAND | wxALL, constants::zero_margin_padding);

    SetSizer(main_sizer_ptr);
    Layout();
}

void MainFrame::BindActionEvents() noexcept {
    Bind(wxEVT_MENU, &MainFrame::OnExitAction, this, static_cast<int>(MenuId::ExitId));
    Bind(wxEVT_MENU, &MainFrame::OnAboutAction, this, static_cast<int>(MenuId::AboutId));
    Bind(wxEVT_MENU, &MainFrame::OnLicenceAction, this, static_cast<int>(MenuId::LicenceId));
    
    Bind(EVT_MALAMA_TOKEN, &MainFrame::OnTokenReceived);
}

void MainFrame::OnExitAction([[maybe_unused]] wxCommandEvent &event) {
    Close(true);
}

void MainFrame::OnAboutAction([[maybe_unused]] wxCommandEvent &event) {
    wxMessageBox(
        "malama Native Local LLM Interface Client\n"
        "Version 0.0.5\n\n"
        "Engineered with C++23 & Native wxWidgets 3.3 for Linux systems.",
        "About malama", 
        wxOK | wxICON_INFORMATION, 
        this
    );
}

void MainFrame::OnLicenceAction([[maybe_unused]] wxCommandEvent &event) {
    wxMessageBox(
        "Licensed under the Apache License, Version 2.0 (the \"License\");\n"
        "you may not use this file except in compliance with the License.\n"
        "You may obtain a copy of the License at:\n\n"
        "http://www.apache.org/licenses/LICENSE-2.0",
        "Licence Constraints", 
        wxOK | wxICON_INFORMATION, 
        this
    );
}

void MainFrame::OnTokenReceived(wxThreadEvent &event) {
    const auto serialized_token = event.GetString().ToStdString(wxConvUTF8);
    spdlog::trace("UI Thread received background token segment: {}", serialized_token);
}

} // namespace malama::ui

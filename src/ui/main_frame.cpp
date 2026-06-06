// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/main_frame.cpp
// Purpose:     Implements top-level window controls and menu modal dialog loops
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-06
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "ui/main_frame.hpp"

#include <wx/menu.h>
#include <wx/string.h>
#include <wx/msgdlg.h>
#include <wx/event.h>
#include <new>

namespace malama::ui {

MainFrame::MainFrame(const wxString &title, const wxPoint &pos, const wxSize &size)
    : wxFrame(nullptr, wxID_ANY, title, pos, size) {
    SetupMenuBar();
    BindActionEvents();
}

void MainFrame::SetupMenuBar() noexcept {
    // Fixed: Added NOLINT checks for framework-level raw memory handoffs
    auto *menu_bar_ptr = new(std::nothrow) wxMenuBar(); // NOLINT(cppcoreguidelines-owning-memory)

    // File Action Selection Menu
    auto *file_menu_ptr = new(std::nothrow) wxMenu(); // NOLINT(cppcoreguidelines-owning-memory)
    file_menu_ptr->Append(static_cast<int>(MenuId::ExitId), "E&xit\tAlt-X", "Terminate application framework");
    menu_bar_ptr->Append(file_menu_ptr, "&File");

    // Help Action Selection Menu
    auto *help_menu_ptr = new(std::nothrow) wxMenu(); // NOLINT(cppcoreguidelines-owning-memory)
    help_menu_ptr->Append(static_cast<int>(MenuId::LicenceId), "&Licence", "Display open-source licensing constraints");
    help_menu_ptr->Append(static_cast<int>(MenuId::AboutId), "&About...", "Display platform implementation details");
    menu_bar_ptr->Append(help_menu_ptr, "&Help");

    SetMenuBar(menu_bar_ptr);
}

void MainFrame::BindActionEvents() noexcept {
    Bind(wxEVT_MENU, &MainFrame::OnExitAction, this, static_cast<int>(MenuId::ExitId));
    Bind(wxEVT_MENU, &MainFrame::OnAboutAction, this, static_cast<int>(MenuId::AboutId));
    Bind(wxEVT_MENU, &MainFrame::OnLicenceAction, this, static_cast<int>(MenuId::LicenceId));
}

// Fixed: Explicitly named parameters with [[maybe_unused]] attributes to satisfy all linter rules
void MainFrame::OnExitAction([[maybe_unused]] wxCommandEvent &event) {
    Close(true);
}

void MainFrame::OnAboutAction([[maybe_unused]] wxCommandEvent &event) {
    wxMessageBox(
        "malama Native Local LLM Interface Client\n"
        "Version 0.0.2\n\n"
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

} // namespace malama::ui

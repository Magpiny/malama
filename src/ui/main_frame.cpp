// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/main_frame.cpp
// Purpose:     Implements top-level window controls and menu modal dialog loops
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-09
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
    // Instantiate the interactive splitter canvas container directly as a child of the Frame
    m_splitter_window_ptr = new (std::nothrow) wxSplitterWindow(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D
    );
    if (m_splitter_window_ptr == nullptr) {
        return;
    }

    // Allocate content panes under the parent splitter container hierarchy
    m_sidebar_panel_ptr = new (std::nothrow) SidebarPanel(m_splitter_window_ptr);
    if (m_sidebar_panel_ptr == nullptr) {
        delete m_splitter_window_ptr; // NOLINT(cppcoreguidelines-owning-memory)
        return;
    }

    m_chat_panel_ptr = new (std::nothrow) ChatPanel(m_splitter_window_ptr);
    if (m_chat_panel_ptr == nullptr) {
        delete m_sidebar_panel_ptr;  // NOLINT(cppcoreguidelines-owning-memory)
        delete m_splitter_window_ptr; // NOLINT(cppcoreguidelines-owning-memory)
        return;
    }

    // Configure pane constraint minimum boundaries to prevent collapse clipping zones
    m_splitter_window_ptr->SetMinimumPaneSize(constants::minimum_pane_size_pixels);

    // Bind layout elements horizontally along an adjustable sash interface split layout anchor
    m_splitter_window_ptr->SplitVertically(m_sidebar_panel_ptr, m_chat_panel_ptr, constants::default_sash_position);
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
        "Version 0.0.7\n\n"
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
    // Extract parent reference safely from the routing notification context payload wrapper
    auto *frame_ptr = wxDynamicCast(event.GetEventObject(), MainFrame);
    if (frame_ptr == nullptr || frame_ptr->m_chat_panel_ptr == nullptr) {
        return;
    }

    const auto raw_token = event.GetString().ToStdString(wxConvUTF8);
    
    // Push the string into the live view terminal control
    frame_ptr->m_chat_panel_ptr->AppendToken(raw_token);
}

} // namespace malama::ui

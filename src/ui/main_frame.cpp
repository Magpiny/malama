// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/main_frame.cpp
// Purpose:     Implements top-level window controls and menu modal dialog loops
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-15
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "ui/main_frame.hpp"
#include "ui/sidebar_panel.hpp"
#include "ui/chat_panel.hpp"
#include "ui/settings_dialog.hpp" // NEW: Required to instantiate the dialog
#include "common/constants.hpp"

#include <new>
#include <string>
#include <wx/menu.h>
#include <wx/msgdlg.h>

namespace malama::ui {

wxDEFINE_EVENT(EVT_MALAMA_TOKEN, wxThreadEvent);
wxDECLARE_EVENT(EVT_USER_PROMPT, wxCommandEvent);

MainFrame::MainFrame(
    const wxString &title, 
    const wxPoint &pos, 
    const wxSize &size, 
    std::function<void(const std::string&)> on_prompt_submit
)
    : wxFrame(nullptr, wxID_ANY, title, pos, size), 
      m_on_prompt_submit_callback(std::move(on_prompt_submit)) 
{
    SetupMenuBar();
    SetupWorkspaceLayout();
    BindActionEvents();
}

auto MainFrame::AppendUserMessage(std::string_view message) noexcept -> void {
    if (m_chat_panel_ptr != nullptr) {
        m_chat_panel_ptr->append_user_message(message);
    }
}

auto MainFrame::AppendToken(std::string_view token_segment) noexcept -> void {
    if (m_chat_panel_ptr != nullptr) {
        m_chat_panel_ptr->append_token(token_segment);
    }
}

void MainFrame::SetupMenuBar() noexcept {
    wxMenuBar *menu_bar_ptr = new (std::nothrow) wxMenuBar(); // NOLINT
    if (menu_bar_ptr == nullptr) {return;}

    auto *file_menu_ptr = new (std::nothrow) wxMenu(); // NOLINT
    if (file_menu_ptr == nullptr) { delete menu_bar_ptr; return; } // NOLINT
    
    // NEW: Added Settings menu item and separator before Exit
    file_menu_ptr->Append(static_cast<int>(MenuId::PreferencesId), "Settings...\tCtrl+,", "Configure malama settings");
    file_menu_ptr->AppendSeparator();
    file_menu_ptr->Append(static_cast<int>(MenuId::ExitId), "E&xit\tAlt-X", "Terminate application framework");
    menu_bar_ptr->Append(file_menu_ptr, "&File");

    auto *help_menu_ptr = new (std::nothrow) wxMenu(); // NOLINT
    if (help_menu_ptr == nullptr) { delete menu_bar_ptr; return; } // NOLINT
    help_menu_ptr->Append(static_cast<int>(MenuId::LicenceId), "&Licence", "Display open-source licensing constraints");
    help_menu_ptr->Append(static_cast<int>(MenuId::AboutId), "&About...", "Display platform implementation details");
    menu_bar_ptr->Append(help_menu_ptr, "&Help");

    SetMenuBar(menu_bar_ptr);
}

void MainFrame::SetupWorkspaceLayout() noexcept {
    // 1. Create a scoped owner variable to satisfy the linter
    auto *splitter = new (std::nothrow) wxSplitterWindow(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D
    );
    
    // 2. Assign the owner to your class member
    m_splitter_window_ptr = splitter;
    if (m_splitter_window_ptr == nullptr) {return;}

    // Repeat for panels
    auto *sidebar = new (std::nothrow) SidebarPanel(m_splitter_window_ptr);
    m_sidebar_panel_ptr = sidebar;
    if (m_sidebar_panel_ptr == nullptr) { 
        delete m_splitter_window_ptr; 
        return; 
    }

    auto *chat = new (std::nothrow) ChatPanel(m_splitter_window_ptr);
    m_chat_panel_ptr = chat;
    if (m_chat_panel_ptr == nullptr) { 
        delete m_sidebar_panel_ptr; 
        delete m_splitter_window_ptr; 
        return; 
    }

    m_splitter_window_ptr->SetMinimumPaneSize(constants::minimum_pane_size_pixels);
    m_splitter_window_ptr->SplitVertically(m_sidebar_panel_ptr, m_chat_panel_ptr, constants::default_sash_position);
}

void MainFrame::BindActionEvents() noexcept {
    // NEW: Bound the Preferences Action
    Bind(wxEVT_MENU, &MainFrame::OnPreferencesAction, this, static_cast<int>(MenuId::PreferencesId));
    
    Bind(wxEVT_MENU, &MainFrame::OnExitAction, this, static_cast<int>(MenuId::ExitId));
    Bind(wxEVT_MENU, &MainFrame::OnAboutAction, this, static_cast<int>(MenuId::AboutId));
    Bind(wxEVT_MENU, &MainFrame::OnLicenceAction, this, static_cast<int>(MenuId::LicenceId));
    Bind(EVT_USER_PROMPT, &MainFrame::OnUserPromptSubmitted, this);
}

// NEW: Implementation for spawning the Settings Dialog
void MainFrame::OnPreferencesAction([[maybe_unused]] wxCommandEvent &event) noexcept {
    SettingsDialog dialog(this);
    dialog.ShowModal();
}

void MainFrame::OnExitAction([[maybe_unused]] wxCommandEvent &event) noexcept {
    Close(true);
}

void MainFrame::OnAboutAction([[maybe_unused]] wxCommandEvent &event) noexcept {
    wxMessageBox(
        "malama Native Local LLM Interface Client\n"
        "Version 0.1.1 (MVP)\n\n"
        "Engineered with C++23 & Native wxWidgets 3.3 for Linux systems.",
        "About malama", 
        wxOK | wxICON_INFORMATION, 
        this
    );
}

void MainFrame::OnLicenceAction([[maybe_unused]] wxCommandEvent &event) noexcept {
    wxMessageBox(
        "Licensed under the Apache License, Version 2.0\n",
        "Licence Constraints", 
        wxOK | wxICON_INFORMATION, 
        this
    );
}

void MainFrame::OnUserPromptSubmitted(wxCommandEvent &event) noexcept {
    if (m_on_prompt_submit_callback) {
        m_on_prompt_submit_callback(event.GetString().ToStdString(wxConvUTF8));
    }
}

} // namespace malama::ui

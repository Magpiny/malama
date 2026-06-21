// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/main_frame.cpp
// Purpose:     Implements top-level window controls and menu modal dialog loops
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-15
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/main_frame.hpp"
#include "ui/sidebar_panel.hpp"
#include "ui/chat_panel.hpp"
#include "ui/settings_dialog.hpp"
#include "common/constants.hpp"

#include <new>
#include <string>
#include <filesystem>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>

namespace malama::ui {
using std::filesystem::path;

wxDEFINE_EVENT(EVT_MALAMA_TOKEN, wxThreadEvent);
wxDECLARE_EVENT(EVT_USER_PROMPT, wxCommandEvent);

inline constexpr int id_menu_preferences = 12001;
inline constexpr int id_menu_exit        = 12002;
inline constexpr int id_menu_about       = 12003;
inline constexpr int id_menu_licence     = 12004;

MainFrame::MainFrame(
    const wxString &title, 
    const wxPoint &pos, 
    const wxSize &size, 
    std::function<void(const std::string&)> on_prompt_submit
)
    : wxFrame(nullptr, wxID_ANY, title, pos, size), 
      m_on_prompt_submit_callback(std::move(on_prompt_submit)) 
{
    const char* home_dir = std::getenv("HOME");
    path app_data_dir =
        (home_dir != nullptr)
            ? std::filesystem::path(home_dir) / ".local" / "share" / "malama"
            : std::filesystem::path("/tmp/malama");

    std::error_code ec;
    std::filesystem::create_directories(app_data_dir / "sessions", ec);

    m_history_manager_ptr = std::make_unique<engine::storage::HistoryManager>(app_data_dir / "sessions");

    setup_menu_bar();
    setup_workspace_layout();
    bind_action_events();
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

void MainFrame::setup_menu_bar() noexcept {
    wxMenuBar *menu_bar_ptr = new (std::nothrow) wxMenuBar(); // NOLINT
    if (menu_bar_ptr == nullptr) { return; }

    auto *file_menu_ptr = new (std::nothrow) wxMenu(); // NOLINT
    if (file_menu_ptr == nullptr) { delete menu_bar_ptr; return; } // NOLINT
    
    file_menu_ptr->Append(id_menu_preferences, "Settings...\tCtrl+,", "Configure malama settings");
    file_menu_ptr->AppendSeparator();
    file_menu_ptr->Append(id_menu_exit, "Exit\tAlt-X", "Terminate application framework");
    menu_bar_ptr->Append(file_menu_ptr, "&File");

    auto *help_menu_ptr = new (std::nothrow) wxMenu(); // NOLINT
    if (help_menu_ptr == nullptr) { delete menu_bar_ptr; return; } // NOLINT
    help_menu_ptr->Append(id_menu_licence, "Licence", "Display open-source licensing constraints");
    help_menu_ptr->Append(id_menu_about, "About...", "Display platform implementation details");
    menu_bar_ptr->Append(help_menu_ptr, "&Help");

    SetMenuBar(menu_bar_ptr);
}

void MainFrame::setup_workspace_layout() noexcept {
    auto *splitter = new (std::nothrow) wxSplitterWindow(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D
    );
    
    m_splitter_window_ptr = splitter;
    if (m_splitter_window_ptr == nullptr) { return; }

    auto *sidebar = new (std::nothrow) SidebarPanel(m_splitter_window_ptr, m_history_manager_ptr.get());
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

void MainFrame::bind_action_events() noexcept {
    Bind(wxEVT_MENU, &MainFrame::on_preferences_action, this, id_menu_preferences);
    Bind(wxEVT_MENU, &MainFrame::on_exit_action, this, id_menu_exit);
    Bind(wxEVT_MENU, &MainFrame::on_about_action, this, id_menu_about);
    Bind(wxEVT_MENU, &MainFrame::on_licence_action, this, id_menu_licence);
    Bind(EVT_USER_PROMPT, &MainFrame::on_user_prompt_submitted, this);
    Bind(EVT_LOAD_SESSION, &MainFrame::on_load_session, this);
    Bind(EVT_NEW_CHAT_REQUESTED, &MainFrame::on_new_chat_action, this);
}

void MainFrame::on_load_session(wxCommandEvent& event) noexcept {
    m_current_session_id = event.GetString().ToStdString();
    
    if (m_history_manager_ptr && (m_chat_panel_ptr != nullptr)) {
        auto session_opt = m_history_manager_ptr->LoadSession(m_current_session_id);
        if (session_opt.has_value()) {
            m_chat_panel_ptr->load_history(session_opt.value());
        }
    }
}

void MainFrame::on_preferences_action([[maybe_unused]] wxCommandEvent &event) noexcept {
    SettingsDialog dialog(this);
    dialog.ShowModal();
}

void MainFrame::on_exit_action([[maybe_unused]] wxCommandEvent &event) noexcept {
    Close(true);
}

void MainFrame::on_about_action([[maybe_unused]] wxCommandEvent &event) noexcept {
    wxMessageBox(
        "malama Native Local LLM Interface Client\n"
        "Version 0.2.0\n\n"
        "Engineered with C++23 & Native wxWidgets for Linux systems.",
        "About malama", 
        wxOK | wxICON_INFORMATION, 
        this
    );
}

void MainFrame::on_licence_action([[maybe_unused]] wxCommandEvent &event) noexcept {
    wxMessageBox(
        "Distributed under the GNU General Public License v3.0\n",
        "Licence Constraints", 
        wxOK | wxICON_INFORMATION, 
        this
    );
}

void MainFrame::on_user_prompt_submitted(wxCommandEvent &event) noexcept {
    if (m_history_manager_ptr == nullptr) return;

    std::string prompt_text = event.GetString().ToStdString(wxConvUTF8);
    if (prompt_text.empty()) return;

    if (m_current_session_id.empty()) {
        std::string default_title = prompt_text.length() > 25 
                                  ? prompt_text.substr(0, 25) + "..." 
                                  : prompt_text;

        auto new_session = m_history_manager_ptr->CreateSession(default_title);
        m_current_session_id = new_session.m_session_id;

        // FIXED: Force history manager to sync file index structures right before updating UI
        const char* home_dir = std::getenv("HOME");
        path app_data_dir = (home_dir != nullptr) ? path(home_dir) / ".local" / "share" / "malama" : path("/tmp/malama");
        m_history_manager_ptr = std::make_unique<engine::storage::HistoryManager>(app_data_dir / "sessions");

        if (m_sidebar_panel_ptr != nullptr) {
            m_sidebar_panel_ptr->populate_sidebar();
        }
    }

    core::Message user_msg;
    user_msg.m_id = engine::storage::HistoryManager::GenerateUuidString();
    user_msg.m_role = core::MessageRole::User;
    user_msg.m_content = prompt_text;
    user_msg.m_timestamp = engine::storage::HistoryManager::GetCurrentEpoch();
    user_msg.m_is_starred = false;

    m_history_manager_ptr->AppendMessage(m_current_session_id, user_msg);

    if (m_on_prompt_submit_callback) {
        m_on_prompt_submit_callback(prompt_text);
    }
}

void MainFrame::on_new_chat_action([[maybe_unused]] wxCommandEvent &event) noexcept {
    m_current_session_id.clear();
    if (m_chat_panel_ptr != nullptr) {
        m_chat_panel_ptr->load_history(core::ChatSession{}); 
    }
}

} // namespace malama::ui

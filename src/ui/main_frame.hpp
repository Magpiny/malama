// /////////////////////////////////////////////////////////////////////////////
// Name:        include/ui/main_frame.hpp
// Purpose:     Top-level workspace frame containing native menu actions
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <wx/wx.h>
#include <wx/splitter.h>
#include "engine/storage/history_manager.hpp"

namespace malama::ui {

class SidebarPanel;
class ChatPanel;

wxDECLARE_EVENT(EVT_MALAMA_TOKEN, wxThreadEvent);

class MainFrame final : public wxFrame {
public:
    explicit MainFrame(
        const wxString &title, 
        const wxPoint &pos, 
        const wxSize &size, 
        std::function<void(const std::string&)> on_prompt_submit
    );
    ~MainFrame() override = default;

    MainFrame(const MainFrame &) = delete;
    MainFrame &operator=(const MainFrame &) = delete;
    MainFrame(MainFrame &&) noexcept = delete;
    MainFrame &operator=(MainFrame &&) noexcept = delete;

    auto AppendUserMessage(std::string_view message) noexcept -> void;
    auto AppendToken(std::string_view token_segment) noexcept -> void;

private:
    void on_load_session(wxCommandEvent& event) noexcept;

    std::unique_ptr<engine::storage::HistoryManager> m_history_manager_ptr;
    std::string m_current_session_id; // Tracks what chat we are actively in

    // Core Layout & Bindings
    void setup_workspace_layout() noexcept;
    void bind_action_events() noexcept;
    void setup_menu_bar() noexcept;

    // Event Handlers
    void on_preferences_action(wxCommandEvent &event) noexcept;
    void on_exit_action(wxCommandEvent &event) noexcept;
    void on_about_action(wxCommandEvent &event) noexcept;
    void on_licence_action(wxCommandEvent &event) noexcept;
    void on_user_prompt_submitted(wxCommandEvent &event) noexcept;
    void on_new_chat_action(wxCommandEvent &event) noexcept;

    enum class MenuId : std::uint16_t {
        PreferencesId = wxID_HIGHEST + 10,
        ExitId = wxID_HIGHEST + 11,
        AboutId = wxID_HIGHEST + 12,
        LicenceId = wxID_HIGHEST + 13
    };

    wxSplitterWindow *m_splitter_window_ptr{nullptr};
    SidebarPanel *m_sidebar_panel_ptr{nullptr};
    ChatPanel *m_chat_panel_ptr{nullptr};

    std::function<void(const std::string&)> m_on_prompt_submit_callback;
};

} // namespace malama::ui

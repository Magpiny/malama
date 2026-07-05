// /////////////////////////////////////////////////////////////////////////////
// Name:        include/ui/main_frame.hpp
// Purpose:     Top-level workspace frame containing native menu actions
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <wx/splitter.h>
#include <wx/wx.h>

#include "engine/storage/history_manager.hpp"

namespace malama::ui {

class SidebarPanel;
class ChatPanel;

wxDECLARE_EVENT(EVT_MALAMA_TOKEN, wxThreadEvent);

class MainFrame final : public wxFrame {
   public:
    explicit MainFrame(const wxString &title, const wxPoint &pos, const wxSize &size,
                       std::function<void(const std::string &)> on_prompt_submit);
    ~MainFrame() override = default;

    MainFrame(const MainFrame &) = delete;
    auto operator=(const MainFrame &) -> MainFrame & = delete;
    MainFrame(MainFrame &&) noexcept = delete;
    auto operator=(MainFrame &&) noexcept -> MainFrame & = delete;

    auto AppendUserMessage(std::string_view message) noexcept -> void;
    auto AppendToken(std::string_view token_segment) noexcept -> void;
    void FinalizeAssistantResponse() noexcept;

   private:
    void on_load_session(wxCommandEvent &WXUNUSED(event)) noexcept;
    void LoadMostRecentSessionOnStartup() noexcept;

    std::unique_ptr<engine::storage::HistoryManager> m_history_manager_ptr;
    std::string m_current_session_id;

    // Core Layout & Bindings
    void setup_workspace_layout() noexcept;
    void bind_action_events() noexcept;
    void setup_menu_bar() noexcept;

    // FIXED: Layout synchronization handler to dynamically apply colors and fonts
    void apply_appearance_settings() noexcept;

    // Event Handlers
    void on_preferences_action(wxCommandEvent &WXUNUSED(event)) noexcept;
    void on_exit_action(wxCommandEvent &WXUNUSED(event)) noexcept;
    void on_about_action(wxCommandEvent &WXUNUSED(event)) noexcept;
    void on_licence_action(wxCommandEvent &WXUNUSED(event)) noexcept;
    void on_user_prompt_submitted(wxCommandEvent &WXUNUSED(event)) noexcept;
    void on_new_chat_action(wxCommandEvent &WXUNUSED(event)) noexcept;

    enum class MenuId : std::uint16_t {
        PreferencesId = wxID_HIGHEST + 10,
        ExitId = wxID_HIGHEST + 11,
        AboutId = wxID_HIGHEST + 12,
        LicenceId = wxID_HIGHEST + 13
    };

    wxSplitterWindow *m_splitter_window_ptr{nullptr};
    SidebarPanel *m_sidebar_panel_ptr{nullptr};
    ChatPanel *m_chat_panel_ptr{nullptr};

    std::function<void(const std::string &)> m_on_prompt_submit_callback;
};

}  // namespace malama::ui

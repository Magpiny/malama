// /////////////////////////////////////////////////////////////////////////////
// Name:        include/ui/main_frame.hpp
// Purpose:     Top-level application main window header with session parameter support
// Author:      Wanjare S <samuelwanjare@proton.me>
// Created:     2026-06-15
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <wx/frame.h>
#include <wx/splitter.h>

#include "common/types.hpp"
#include "core/models.hpp"
#include "engine/storage/history_manager.hpp"

namespace malama::ui {

class ChatPanel;
class SidebarPanel;

wxDECLARE_EVENT(EVT_MALAMA_TOKEN, wxThreadEvent);

/**
 * @class MainFrame
 * @brief Primary GTK top-level window managing session routing, layout splits, and menu dialogs.
 */
class MainFrame final : public wxFrame {
   public:
    explicit MainFrame(const wxString &title, const wxPoint &pos, const wxSize &size,
                       std::function<void(const std::string &)> on_prompt_submit);

    ~MainFrame() override = default;

    MainFrame(const MainFrame &) = delete;
    MainFrame &operator=(const MainFrame &) = delete;
    MainFrame(MainFrame &&) = delete;
    MainFrame &operator=(MainFrame &&) = delete;

    void AppendUserMessage(std::string_view message) noexcept;
    void AppendToken(std::string_view token_segment) noexcept;
    void FinalizeAssistantResponse() noexcept;

    [[nodiscard]] common::SessionParameters GetActiveSessionParameters() const noexcept {
        return m_current_session_params;
    }

    [[nodiscard]] std::vector<core::Message> GetActiveSessionMessages() const noexcept;

   private:
    void setup_menu_bar() noexcept;
    void setup_workspace_layout() noexcept;
    void bind_action_events() noexcept;
    void load_application_icon() noexcept;
    void apply_appearance_settings() noexcept;
    void LoadMostRecentSessionOnStartup() noexcept;

    // Action Handlers
    void on_preferences_action(wxCommandEvent &event) noexcept;
    void on_session_params_action(wxCommandEvent &event) noexcept;
    void on_exit_action(wxCommandEvent &event) noexcept;
    void on_about_action(wxCommandEvent &event) noexcept;
    void on_licence_action(wxCommandEvent &event) noexcept;
    void on_user_prompt_submitted(wxCommandEvent &event) noexcept;
    void on_load_session(wxCommandEvent &event) noexcept;
    void on_new_chat_action(wxCommandEvent &event) noexcept;

    void on_export_session_action(wxCommandEvent &WXUNUSED(event));

    // UI Widgets & Subsystems
    wxSplitterWindow *m_splitter_window_ptr{nullptr};
    SidebarPanel *m_sidebar_panel_ptr{nullptr};
    ChatPanel *m_chat_panel_ptr{nullptr};

    std::unique_ptr<engine::storage::HistoryManager> m_history_manager_ptr{nullptr};
    std::string m_current_session_id;
    common::SessionParameters m_current_session_params{};

    std::function<void(const std::string &)> m_on_prompt_submit_callback;
};

}  // namespace malama::ui

// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/main_frame.hpp
// Purpose:     Top-level workspace frame containing native menu actions
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <functional>
#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <wx/splitter.h>
#include "engine/storage/history_manager.hpp"

namespace malama::engine::storage { class HistoryManager; }

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
    void OnLoadSession(wxCommandEvent& event) noexcept;

    std::unique_ptr<engine::storage::HistoryManager> m_history_manager_ptr;
    std::string m_current_session_id; // Tracks what chat we are actively in

    // Core Layout & Bindings
    void SetupWorkspaceLayout() noexcept;
    void BindActionEvents() noexcept;
    void SetupMenuBar() noexcept; // Merged with setup_menu()

    // Event Handlers (
    void OnPreferencesAction(wxCommandEvent &event) noexcept; // Renamed from on_preferences
    void OnExitAction([[maybe_unused]] wxCommandEvent &event) noexcept;
    void OnAboutAction([[maybe_unused]] wxCommandEvent &event) noexcept;
    void OnLicenceAction([[maybe_unused]] wxCommandEvent &event) noexcept;
    void OnUserPromptSubmitted(wxCommandEvent &event) noexcept;
    void OnNewChatAction([[maybe_unused]] wxCommandEvent &event) noexcept;

    // TitleCase for Enums
    enum class MenuId : std::uint16_t {
        PreferencesId = wxID_PREFERENCES, // Added the standard preference ID
        ExitId = wxID_EXIT,
        AboutId = wxID_ABOUT,
        LicenceId = wxID_HIGHEST + 1
    };

    wxSplitterWindow *m_splitter_window_ptr{nullptr};
    SidebarPanel *m_sidebar_panel_ptr{nullptr};
    ChatPanel *m_chat_panel_ptr{nullptr};

    std::function<void(const std::string&)> m_on_prompt_submit_callback;
};

} // namespace malama::ui

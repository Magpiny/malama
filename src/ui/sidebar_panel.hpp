// /////////////////////////////////////////////////////////////////////////////
// Name:        include/ui/sidebar_panel.hpp
// Purpose:     Sidebar control mechanics and history navigation
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/wx.h>
#include <vector>
#include "core/models.hpp"

namespace malama::engine::storage { class HistoryManager; }

namespace malama::ui {

wxDECLARE_EVENT(EVT_LOAD_SESSION, wxCommandEvent);
wxDECLARE_EVENT(EVT_NEW_CHAT_REQUESTED, wxCommandEvent); // NEW: Notify MainFrame to reset

class SidebarPanel final : public wxPanel {
public:
    explicit SidebarPanel(wxWindow *parent_ptr, engine::storage::HistoryManager *history_manager_ptr = nullptr);
    ~SidebarPanel() override = default;

    SidebarPanel(const SidebarPanel &) = delete;
    SidebarPanel &operator=(const SidebarPanel &) = delete;
    SidebarPanel(SidebarPanel &&) noexcept = delete;
    /**
 * @brief SidebarPanel cannot be move-assigned.
 */
SidebarPanel &operator=(SidebarPanel &&) noexcept = delete;

    void populate_sidebar() noexcept;

private:
    void setup_layout() noexcept;
    void bind_events() noexcept;

    void on_session_selected(wxCommandEvent &event) noexcept;
    void on_context_menu(wxContextMenuEvent &event) noexcept;
    void on_new_chat_click(wxCommandEvent &event) noexcept; // NEW: Private snake_case handler

    wxButton *m_new_chat_btn_ptr{nullptr}; // NEW: Button tracker
    wxListBox *m_history_list_ptr{nullptr};
    engine::storage::HistoryManager *m_history_manager_ptr{nullptr};
    
    std::vector<core::SessionMetadata> m_active_metadata; 
};

} // namespace malama::ui

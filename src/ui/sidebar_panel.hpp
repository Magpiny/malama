// /////////////////////////////////////////////////////////////////////////////
// Name:        include/ui/sidebar_panel.hpp
// Purpose:     Sidebar control mechanics and history navigation
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <wx/event.h>
#include <wx/stattext.h>
#include <wx/wx.h>

#include "core/models.hpp"

namespace malama::engine::storage {
class HistoryManager;
}

namespace malama::ui {

wxDECLARE_EVENT(EVT_LOAD_SESSION, wxCommandEvent);
wxDECLARE_EVENT(EVT_NEW_CHAT_REQUESTED, wxCommandEvent);

class SidebarPanel final : public wxPanel {
   public:
    explicit SidebarPanel(wxWindow *parent_ptr,
                          engine::storage::HistoryManager *history_manager_ptr = nullptr);
    ~SidebarPanel() override = default;

    SidebarPanel(const SidebarPanel &) = delete;
    auto operator=(const SidebarPanel &) -> SidebarPanel & = delete;
    SidebarPanel(SidebarPanel &&) noexcept = delete;
    auto operator=(SidebarPanel &&) noexcept -> SidebarPanel & = delete;

    void populate_sidebar() noexcept;
    void select_session_by_id(const std::string &target_id) noexcept;

    void update_metrics(double tokens_per_second, int percentage) noexcept;
    void clear_metrics() noexcept;

   private:
    void setup_layout() noexcept;
    void bind_events() noexcept;

    void on_session_selected(wxCommandEvent &custom_event) noexcept;
    void on_context_menu(wxContextMenuEvent &context_event) noexcept;
    void on_new_chat_click(wxCommandEvent &button_event) noexcept;

    // Appended coordinate tracking listener declaration signature
    void on_history_mouse_motion(wxMouseEvent &mouse_event) noexcept;

    wxButton *m_new_chat_btn_ptr{nullptr};
    wxListBox *m_history_list_ptr{nullptr};
    wxStaticText *m_metrics_text_ptr{nullptr};
    engine::storage::HistoryManager *m_history_manager_ptr{nullptr};

    std::vector<core::SessionMetadata> m_active_metadata;
};

}  // namespace malama::ui

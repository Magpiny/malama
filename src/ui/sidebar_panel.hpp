// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/sidebar_panel.hpp
// Purpose:     Sidebar control mechanics and history navigation
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/wx.h>
#include <vector>
#include "core/models.hpp" // Assuming this holds core::SessionMetadata

// Forward declaration to prevent heavy includes in the header
namespace malama::engine::storage { class HistoryManager; }

namespace malama::ui {

wxDECLARE_EVENT(EVT_LOAD_SESSION, wxCommandEvent);

class SidebarPanel final : public wxPanel {
public:
    // Inject the HistoryManager via pointer so the sidebar can read/write state
    explicit SidebarPanel(wxWindow *parent_ptr, engine::storage::HistoryManager *history_manager_ptr = nullptr);
    ~SidebarPanel() override = default;

    SidebarPanel(const SidebarPanel &) = delete;
    SidebarPanel &operator=(const SidebarPanel &) = delete;
    SidebarPanel(SidebarPanel &&) noexcept = delete;
    SidebarPanel &operator=(SidebarPanel &&) noexcept = delete;

    void PopulateSidebar() noexcept;

private:
    void SetupLayout() noexcept;
    void BindEvents() noexcept;

    void OnSessionSelected(wxCommandEvent &event) noexcept;
    void OnContextMenu(wxContextMenuEvent &event) noexcept;

    wxListBox *m_history_list_ptr{nullptr};
    engine::storage::HistoryManager *m_history_manager_ptr{nullptr};
    
    // Low-memory cache of the sidebar state
    std::vector<core::SessionMetadata> m_active_metadata; 
};

} // namespace malama::ui

// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/sidebar_panel.cpp
// Purpose:     Implements the structural sidebar control mechanics
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "ui/sidebar_panel.hpp"
#include "common/constants.hpp"
#include "engine/storage/history_manager.hpp" // Required for method access

#include <new>
#include <algorithm>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>

namespace malama::ui {

wxDEFINE_EVENT(EVT_LOAD_SESSION, wxCommandEvent);

// Scoped enum for our dynamic context menu
enum class ContextMenuId : std::uint16_t {
    TogglePinId = wxID_HIGHEST + 100,
    RenameId,
    DeleteId
};

SidebarPanel::SidebarPanel(wxWindow *parent_ptr, engine::storage::HistoryManager *history_manager_ptr)
    : wxPanel(parent_ptr, wxID_ANY), m_history_manager_ptr(history_manager_ptr) {
    
    // Apply UI Theming
    SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    
    SetupLayout();
    BindEvents();

    // Only populate if the manager was successfully injected
    if (m_history_manager_ptr != nullptr) {
        PopulateSidebar();
    }
}

void SidebarPanel::SetupLayout() noexcept {
    wxBoxSizer *sizer_ptr = new (std::nothrow) wxBoxSizer(wxVERTICAL); // NOLINT
    if (sizer_ptr == nullptr) {
        return;
    }

    auto *header_ptr = new (std::nothrow) wxStaticText(this, wxID_ANY, "Historical Sessions");
    if (header_ptr == nullptr) {
        delete sizer_ptr; // NOLINT
        return;
    }
    header_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    m_history_list_ptr = new (std::nothrow) wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxBORDER_NONE);
    if (m_history_list_ptr == nullptr) {
        delete header_ptr; // NOLINT
        delete sizer_ptr;  // NOLINT
        return;
    }
    m_history_list_ptr->SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    m_history_list_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    sizer_ptr->Add(header_ptr, constants::layout_proportion_fixed, wxALL | wxEXPAND, constants::default_margin_padding);
    sizer_ptr->Add(m_history_list_ptr, constants::layout_proportion_stretch, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, constants::default_margin_padding);

    SetSizer(sizer_ptr);
}

void SidebarPanel::BindEvents() noexcept {
    if (m_history_list_ptr == nullptr) { return; }

    // Bind left-click to load the session
    m_history_list_ptr->Bind(wxEVT_LISTBOX, &SidebarPanel::OnSessionSelected, this);
    
    // Bind right-click to spawn the context menu
    m_history_list_ptr->Bind(wxEVT_CONTEXT_MENU, &SidebarPanel::OnContextMenu, this);
}

void SidebarPanel::PopulateSidebar() noexcept {
    if (m_history_manager_ptr == nullptr || m_history_list_ptr == nullptr) { return; }

    m_history_list_ptr->Clear();
    m_active_metadata.clear();

    m_active_metadata = m_history_manager_ptr->LoadAllMetadata();

    std::sort(m_active_metadata.begin(), m_active_metadata.end(), [](const core::SessionMetadata &a, const core::SessionMetadata &b) {
        if (a.m_is_pinned != b.m_is_pinned) {
            return a.m_is_pinned > b.m_is_pinned;
        }
        return a.m_updated_at > b.m_updated_at;
    });

    for (const auto &meta : m_active_metadata) {
        std::string display_text = (meta.m_is_pinned ? "📌 " : "💬 ") + meta.m_title;
        m_history_list_ptr->Append(display_text);
    }

    // NEW: Force GTK to repaint the listbox immediately
    m_history_list_ptr->Refresh();
    m_history_list_ptr->Update();
    Layout();
}

void SidebarPanel::OnSessionSelected([[maybe_unused]] wxCommandEvent &event) noexcept {
    int selection_index = m_history_list_ptr->GetSelection();
    if (selection_index == wxNOT_FOUND) { return; }

    wxCommandEvent load_event(EVT_LOAD_SESSION, GetId());
    load_event.SetString(m_active_metadata[selection_index].m_session_id);
    load_event.SetEventObject(this);
    ProcessWindowEvent(load_event);
}

void SidebarPanel::OnContextMenu([[maybe_unused]] wxContextMenuEvent &event) noexcept {
    if (m_history_manager_ptr == nullptr) { return; }

    int selection_index = m_history_list_ptr->GetSelection();
    if (selection_index == wxNOT_FOUND) { return; }

    wxMenu context_menu;
    bool is_pinned = m_active_metadata[selection_index].m_is_pinned;
    
    context_menu.Append(static_cast<int>(ContextMenuId::TogglePinId), is_pinned ? "Unpin Session" : "📌 Pin Session");
    context_menu.Append(static_cast<int>(ContextMenuId::RenameId), "✏️ Rename...");
    context_menu.AppendSeparator();
    context_menu.Append(static_cast<int>(ContextMenuId::DeleteId), "🗑️ Delete");

    // Capture 'this' and 'selection_index' safely to execute the mutations
    context_menu.Bind(wxEVT_MENU, [this, selection_index](wxCommandEvent&) {
        m_history_manager_ptr->ToggleSessionPin(m_active_metadata[selection_index].m_session_id);
        PopulateSidebar(); 
    }, static_cast<int>(ContextMenuId::TogglePinId));

    context_menu.Bind(wxEVT_MENU, [this, selection_index](wxCommandEvent&) {
        wxTextEntryDialog dlg(this, "Enter new session title:", "Rename Session", m_active_metadata[selection_index].m_title);
        if (dlg.ShowModal() == wxID_OK) {
            std::string new_title = dlg.GetValue().ToStdString();
            if (!new_title.empty()) {
                m_history_manager_ptr->UpdateSessionTitle(m_active_metadata[selection_index].m_session_id, new_title);
                PopulateSidebar(); 
            }
        }
    }, static_cast<int>(ContextMenuId::RenameId));

    // Spawn the modal menu at the mouse cursor
    PopupMenu(&context_menu);
}

} // namespace malama::ui

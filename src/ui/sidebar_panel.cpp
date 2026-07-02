// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/sidebar_panel.cpp
// Purpose:     Implements the structural sidebar control mechanics
// Author:      Wanjare S.<samuelwanjare@protonmail.com>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/sidebar_panel.hpp"
#include "common/constants.hpp"
#include "engine/storage/history_manager.hpp"

#include <new>
#include <algorithm>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>

namespace malama::ui {

wxDEFINE_EVENT(EVT_LOAD_SESSION, wxCommandEvent);
wxDEFINE_EVENT(EVT_NEW_CHAT_REQUESTED, wxCommandEvent);

inline constexpr int id_menu_toggle_pin = 11001;
inline constexpr int id_menu_rename     = 11002;
inline constexpr int id_menu_delete     = 11003;
inline constexpr int id_control_new_btn = 11004;

SidebarPanel::SidebarPanel(
    wxWindow *parent_ptr, 
    engine::storage::HistoryManager *history_manager_ptr
) : wxPanel(parent_ptr, wxID_ANY), m_history_manager_ptr(history_manager_ptr) {
    SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    setup_layout();
    bind_events();

    if (m_history_manager_ptr != nullptr) {
        populate_sidebar();
    }
}

void SidebarPanel::setup_layout() noexcept {
    auto *sizer_ptr = new (std::nothrow) wxBoxSizer(wxVERTICAL);
    if (sizer_ptr == nullptr) {
        return;
    }

    auto *header_ptr = new (std::nothrow) wxStaticText(this, wxID_ANY, "Historical Sessions");
    if (header_ptr == nullptr) {
        delete sizer_ptr;
        return;
    }
    header_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    m_new_chat_btn_ptr = new (std::nothrow) wxButton(
        this, id_control_new_btn, "+ New Chat", 
        wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT
    );
    if (m_new_chat_btn_ptr == nullptr) {
        delete header_ptr;
        delete sizer_ptr;
        return;
    }
    m_new_chat_btn_ptr->SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    m_new_chat_btn_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    m_history_list_ptr = new (std::nothrow) wxListBox(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxBORDER_NONE
    );
    if (m_history_list_ptr == nullptr) {
        delete m_new_chat_btn_ptr;
        delete header_ptr;
        delete sizer_ptr;
        return;
    }
    m_history_list_ptr->SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));

    sizer_ptr->Add(
        header_ptr, constants::layout_proportion_fixed, 
        wxALL | wxEXPAND, constants::default_margin_padding
    );
    sizer_ptr->Add(
        m_new_chat_btn_ptr, constants::layout_proportion_fixed, 
        wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, constants::default_margin_padding
    );
    sizer_ptr->Add(
        m_history_list_ptr, constants::layout_proportion_stretch, 
        wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, constants::default_margin_padding
    );

    SetSizer(sizer_ptr);
}

void SidebarPanel::bind_events() noexcept {
    if (m_history_list_ptr == nullptr || m_new_chat_btn_ptr == nullptr) { return; }

    m_history_list_ptr->Bind(wxEVT_LISTBOX, &SidebarPanel::on_session_selected, this);
    m_history_list_ptr->Bind(wxEVT_CONTEXT_MENU, &SidebarPanel::on_context_menu, this);
    m_new_chat_btn_ptr->Bind(wxEVT_BUTTON, &SidebarPanel::on_new_chat_click, this);
}

void SidebarPanel::populate_sidebar() noexcept {
    if (m_history_manager_ptr == nullptr || m_history_list_ptr == nullptr) { return; }

    m_history_list_ptr->Clear();
    m_active_metadata.clear();

    m_history_list_ptr->SetBackgroundColour(wxColour(std::string(constants::color_dark_brown)));
    m_history_list_ptr->SetForegroundColour(wxColour(std::string(constants::color_smoke_white)));

    m_active_metadata = m_history_manager_ptr->LoadAllMetadata();

    std::sort(
        m_active_metadata.begin(), 
        m_active_metadata.end(), 
        [](const core::SessionMetadata &meta_lhs, const core::SessionMetadata &meta_rhs) {
            if (meta_lhs.m_is_pinned != meta_rhs.m_is_pinned) {
                return meta_lhs.m_is_pinned > meta_rhs.m_is_pinned;
            }
            return meta_lhs.m_updated_at > meta_rhs.m_updated_at;
        }
    );

    for (const auto &meta : m_active_metadata) {
        std::string display_text = (meta.m_is_pinned ? "[P] " : "  ") + meta.m_title;
        m_history_list_ptr->Append(wxString::FromUTF8(display_text.c_str()));
    }

    m_history_list_ptr->Refresh();
    m_history_list_ptr->Update();
    Layout();
}

void SidebarPanel::select_session_by_id(const std::string& target_id) noexcept {
    if (m_history_list_ptr == nullptr) {
        return;
    }
    for (std::size_t index = 0; index < m_active_metadata.size(); ++index) {
        if (m_active_metadata[index].m_session_id == target_id) {
            m_history_list_ptr->SetSelection(static_cast<int>(index));
            break;
        }
    }
}

void SidebarPanel::on_new_chat_click([[maybe_unused]] wxCommandEvent &event) noexcept {
    if (m_history_list_ptr->GetSelection() != wxNOT_FOUND) {
        m_history_list_ptr->SetSelection(wxNOT_FOUND);
    }
    m_new_chat_btn_ptr->SetFocus();

    wxCommandEvent clear_event(EVT_NEW_CHAT_REQUESTED, GetId());
    clear_event.SetEventObject(this);
    ProcessWindowEvent(clear_event);
}

void SidebarPanel::on_session_selected([[maybe_unused]] wxCommandEvent &event) noexcept {
    int selection_index = m_history_list_ptr->GetSelection();
    if (selection_index == wxNOT_FOUND || selection_index >= static_cast<int>(m_active_metadata.size())) { 
        return; 
    }

    wxCommandEvent load_event(EVT_LOAD_SESSION, GetId());
    load_event.SetString(m_active_metadata[selection_index].m_session_id);
    load_event.SetEventObject(this);
    ProcessWindowEvent(load_event);
}

void SidebarPanel::on_context_menu([[maybe_unused]] wxContextMenuEvent &event) noexcept {
    if (m_history_manager_ptr == nullptr) { return; }

    int selection_index = m_history_list_ptr->GetSelection();
    if (selection_index == wxNOT_FOUND) { return; }

    wxMenu context_menu;
    bool is_pinned = m_active_metadata[selection_index].m_is_pinned;
    
    context_menu.Append(id_menu_toggle_pin, is_pinned ? "Unpin Session" : "Pin Session");
    context_menu.Append(id_menu_rename, "Rename Session...");
    context_menu.AppendSeparator();
    context_menu.Append(id_menu_delete, "Delete Session");

    context_menu.Bind(wxEVT_MENU, [this, selection_index](wxCommandEvent&) {
        m_history_manager_ptr->ToggleSessionPin(m_active_metadata[selection_index].m_session_id);
        populate_sidebar(); 
    }, id_menu_toggle_pin);

    context_menu.Bind(wxEVT_MENU, [this, selection_index](wxCommandEvent&) {
        wxTextEntryDialog rename_dialog(
            this, "Enter new session title:", "Rename Session", 
            m_active_metadata[selection_index].m_title
        );
        if (rename_dialog.ShowModal() == wxID_OK) {
            std::string new_title = rename_dialog.GetValue().ToStdString();
            if (!new_title.empty()) {
                m_history_manager_ptr->UpdateSessionTitle(
                    m_active_metadata[selection_index].m_session_id, new_title
                );
                populate_sidebar(); 
            }
        }
    }, id_menu_rename);

    context_menu.Bind(wxEVT_MENU, [this, selection_index](wxCommandEvent&) {
        int confirm = wxMessageBox(
            "Are you sure you want to delete this session?", "Confirm Deletion", 
            wxYES_NO | wxICON_WARNING, this
        );
        if (confirm == wxYES) {
            m_history_manager_ptr->DeleteSession(m_active_metadata[selection_index].m_session_id);
            populate_sidebar();
            
            wxCommandEvent reset_event(EVT_NEW_CHAT_REQUESTED, GetId());
            ProcessWindowEvent(reset_event);
        }
    }, id_menu_delete);

    PopupMenu(&context_menu);
}

} // namespace malama::ui

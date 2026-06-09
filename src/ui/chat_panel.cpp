// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.cpp
// Purpose:     Implements the interactive dialogue workspace panel
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-08
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "ui/chat_panel.hpp"
#include "common/constants.hpp"

#include <new>

#include <wx/gtk/textctrl.h>
#include <wx/sizer.h>

namespace malama::ui {

ChatPanel::ChatPanel(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY) {
    SetupLayout();
}

void ChatPanel::SetupLayout() noexcept {
    // Fixed: Added NOLINT check for framework sizer allocations
    auto *sizer_ptr = new(std::nothrow) wxBoxSizer(wxVERTICAL); // NOLINT(cppcoreguidelines-owning-memory)
    if (sizer_ptr == nullptr) {
        return;
    }

    m_chat_display_ptr = new(std::nothrow) wxTextCtrl(
        this, wxID_ANY, "Conversation workspace initialized. Standing by for prompt generation loops...",
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY
    );

    if (m_chat_display_ptr == nullptr) {
        delete sizer_ptr; // NOLINT(cppcoreguidelines-owning-memory)
        return;
    }

    // Fixed: Substituted raw numbers with named padding metrics
    sizer_ptr->Add(m_chat_display_ptr, constants::layout_proportion_stretch, wxALL | wxEXPAND, constants::default_margin_padding);
    SetSizer(sizer_ptr);
}

} // namespace malama::ui

// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.cpp
// Purpose:     Implements the interactive dialogue workspace panel
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "ui/chat_panel.hpp"
#include "common/constants.hpp"

#include <new>

#include <wx/sizer.h>

namespace malama::ui {

ChatPanel::ChatPanel(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY) {
    SetupLayout();
}

void ChatPanel::SetupLayout() noexcept {
    // Explicit type specification to resolve editor linter diagnostic messages completely
    auto *sizer_ptr = new (std::nothrow) wxBoxSizer(wxVERTICAL); // NOLINT(cppcoreguidelines-owning-memory)
    if (sizer_ptr == nullptr) {
        return;
    }

    m_chat_display_ptr = new (std::nothrow) wxTextCtrl(
        this, wxID_ANY, "Conversation workspace initialized. Standing by for token stream...\n",
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY
    );
    if (m_chat_display_ptr == nullptr) {
        delete sizer_ptr; // NOLINT(cppcoreguidelines-owning-memory)
        return;
    }

    sizer_ptr->Add(m_chat_display_ptr, constants::layout_proportion_stretch, wxALL | wxEXPAND, constants::default_margin_padding);
    SetSizer(sizer_ptr);
}

auto ChatPanel::AppendToken(std::string_view token_segment) noexcept -> void {
    if (m_chat_display_ptr == nullptr) [[unlikely]] {
        return;
    }
    // Append the text block and advance the scroll offset caret natively
    m_chat_display_ptr->AppendText(wxString::FromUTF8(token_segment.data(), token_segment.size()));
}

} // namespace malama::ui

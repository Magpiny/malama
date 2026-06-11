// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.hpp
// Purpose:     Primary chat interaction workspace canvas panel
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <string_view>

namespace malama::ui {

class ChatPanel final : public wxPanel {
public:
    explicit ChatPanel(wxWindow *parent_ptr);
    ~ChatPanel() override = default;

    ChatPanel(const ChatPanel &) = delete;
    ChatPanel &operator=(const ChatPanel &) = delete;
    ChatPanel(ChatPanel &&) noexcept = delete;
    ChatPanel &operator=(ChatPanel &&) noexcept = delete;

    auto AppendToken(std::string_view token_segment) noexcept -> void;

private:
    void SetupLayout() noexcept;
    void BindEvents() noexcept;

    // Clipboard event handler
    void OnCopyAction(wxCommandEvent &event) noexcept;

    wxTextCtrl *m_chat_display_ptr{nullptr};
    wxButton *m_copy_button_ptr{nullptr};
};

} // namespace malama::ui

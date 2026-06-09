// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.hpp
// Purpose:     Primary chat interaction workspace canvas panel
// Author:      Wanjare <samuelwanjare@protonmail.com>
// Created:     2026-06-08
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <wx/panel.h>
#include <wx/textctrl.h>

namespace malama::ui {

class ChatPanel final : public wxPanel {
public:
    explicit ChatPanel(wxWindow *parent_ptr);
    ~ChatPanel() override = default;

    ChatPanel(const ChatPanel &) = delete;
    ChatPanel &operator=(const ChatPanel &) = delete;
    ChatPanel(ChatPanel &&) noexcept = delete;
    ChatPanel &operator=(ChatPanel &&) noexcept = delete;

private:
    void SetupLayout() noexcept;

    wxTextCtrl *m_chat_display_ptr=nullptr;
};

} // namespace malama::ui

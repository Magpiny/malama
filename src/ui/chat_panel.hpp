// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.hpp
// Purpose:     Interactive dialogue workspace panel definition
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/html/htmlwin.h>
#include <string>
#include <string_view>

#include "config/config_manager.hpp"
#include "core/models.hpp"

namespace malama::ui {

wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, EVT_USER_PROMPT, wxCommandEvent);

class ChatPanel final : public wxPanel {
public:
    explicit ChatPanel(wxWindow *parent_ptr);
    ~ChatPanel() override = default;

    ChatPanel(const ChatPanel &) = delete;
    auto operator=(const ChatPanel &) -> ChatPanel & = delete;
    ChatPanel(ChatPanel &&) noexcept = delete;
    auto operator=(ChatPanel &&) noexcept -> ChatPanel & = delete;

    void append_token(std::string_view token_segment) noexcept;
    void append_user_message(std::string_view message) noexcept;
    void load_history(const core::ChatSession& session) noexcept;
    [[nodiscard]] auto get_active_response_stream() const noexcept -> std::string;

private:
    void setup_layout() noexcept;
    void bind_events() noexcept;
    void render_chat_stream() noexcept;

    void on_send_action(wxCommandEvent &event) noexcept;
    void on_copy_action(wxCommandEvent &event) noexcept;
    void on_link_clicked(wxHtmlLinkEvent &event) noexcept; 

    wxHtmlWindow *m_chat_display_ptr{nullptr};
    wxTextCtrl *m_prompt_input_ptr{nullptr};
    wxButton *m_send_button_ptr{nullptr};
    wxButton *m_copy_button_ptr{nullptr};

    std::string m_raw_markdown_history;
    std::string m_active_response_stream;
    std::string m_last_llm_response; 
};

} // namespace malama::ui

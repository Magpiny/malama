// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.hpp
// Purpose:     Interactive dialogue workspace panel definition
// Author:      Magpiny <magpinyb@proton.me>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

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

class ChatPanel : public wxPanel {
public:
    explicit ChatPanel(wxWindow *parent_ptr);
    ~ChatPanel() override = default;

    ChatPanel(const ChatPanel &) = delete;
    ChatPanel &operator=(const ChatPanel &) = delete;
    ChatPanel(ChatPanel &&) noexcept = delete;
    ChatPanel &operator=(ChatPanel &&) noexcept = delete;

    auto append_token(std::string_view token_segment) noexcept -> void;
    auto append_user_message(std::string_view message) noexcept -> void;
    auto load_history(const core::ChatSession& session) noexcept -> void;

private:
    auto setup_layout() noexcept -> void;
    auto bind_events() noexcept -> void;
    auto render_chat_stream() noexcept -> void;

    void on_send_action(wxCommandEvent &event) noexcept;
    void on_link_clicked(wxHtmlLinkEvent &event) noexcept; // NEW: Intercepts inline copy button

    wxHtmlWindow *m_chat_display_ptr{nullptr};
    wxHtmlWindow *m_html_window_ptr{nullptr};
    wxTextCtrl *m_prompt_input_ptr{nullptr};
    wxButton *m_send_button_ptr{nullptr};

    std::string m_raw_markdown_history;
    std::string m_active_response_stream;
    std::string m_last_llm_response; // NEW: Stores isolated response for copying
};

} // namespace malama::ui

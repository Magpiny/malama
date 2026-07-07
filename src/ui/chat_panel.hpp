// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/chat_panel.hpp
// Purpose:     Composite multimodal prompt workspace and message canvas layout
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <wx/activityindicator.h>
#include <wx/button.h>
#include <wx/html/htmlwin.h>
#include <wx/panel.h>
#include <wx/textctrl.h>

#include "core/models.hpp"
#include "engine/storage/attachment_manager.hpp"
#include "ui/error_banner.hpp"

// Forward declarations to keep headers lean and avoid compilation loops
class wxBoxSizer;

namespace malama::ui {

wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, EVT_USER_PROMPT, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, EVT_CANCEL_GENERATION, wxCommandEvent);

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
    void load_history(const core::ChatSession &session) noexcept;
    [[nodiscard]] auto get_active_response_stream() const noexcept -> std::string;

    void start_spinner() noexcept;
    void stop_spinner() noexcept;

   private:
    void setup_layout() noexcept;
    void bind_events() noexcept;
    void render_chat_stream() noexcept;
    void refresh_attachment_tray() noexcept;

    void on_send_action(wxCommandEvent &event) noexcept;
    void on_attach_action(wxCommandEvent &event) noexcept;
    void on_copy_action(wxCommandEvent &event) noexcept;
    void on_prompt_key_down(wxKeyEvent &event) noexcept;

    wxHtmlWindow *m_chat_display_ptr{nullptr};
    ErrorBanner *m_error_banner_ptr{nullptr};
    wxBoxSizer *m_tray_sizer_ptr{nullptr};
    wxPanel *m_tray_container_ptr{nullptr};
    wxTextCtrl *m_prompt_input_ptr{nullptr};
    wxButton *m_attach_button_ptr{nullptr};
    wxButton *m_send_button_ptr{nullptr};
    wxButton *m_copy_button_ptr{nullptr};
    wxActivityIndicator *m_spinner_ptr{nullptr};

    std::unique_ptr<engine::storage::AttachmentManager> m_attachment_engine;
    std::string m_raw_markdown_history;
    std::string m_active_response_stream;
    std::string m_last_llm_response;
};

}  // namespace malama::ui

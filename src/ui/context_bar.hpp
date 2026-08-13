// /////////////////////////////////////////////////////////////////////////////
// Name:        include/ui/context_bar.hpp
// Purpose:     Header for the pre-flight context budget usage bar component
// Author:      Wanjare <samuelwanjare@protonmail.com>
// Created:     2026-08-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-later

#include <cstddef>
#include <cstdint>
#include <wx/gauge.h>
#include <wx/panel.h>
#include <wx/stattext.h>

namespace malama::ui {

/**
 * @class ContextBar
 * @brief Visual usage bar indicating current context allocation (Prompt + Attachments vs num_ctx
 * Limit).
 */
class ContextBar final : public wxPanel {
   public:
    /**
     * @brief Construct the ContextBar widget.
     * @param parent Pointer to parent window.
     * @param id Window identifier.
     */
    explicit ContextBar(wxWindow *parent, wxWindowID id = wxID_ANY);

    ~ContextBar() override = default;

    ContextBar(const ContextBar &) = delete;
    ContextBar &operator=(const ContextBar &) = delete;
    ContextBar(ContextBar &&) = delete;
    ContextBar &operator=(ContextBar &&) = delete;

    /**
     * @brief Recalculate and update the pre-flight token usage display.
     * @param prompt_tokens Estimated tokens from prompt text.
     * @param attachment_tokens Estimated tokens from staged file attachments.
     * @param max_context_limit Target session context limit (num_ctx).
     */
    void UpdateBudget(std::size_t prompt_tokens, std::size_t attachment_tokens,
                      std::size_t max_context_limit) noexcept;

   private:
    void InitializeComponents() noexcept;

    wxGauge *m_gauge{nullptr};
    wxStaticText *m_status_label{nullptr};
    wxStaticText *m_warning_label{nullptr};

    std::size_t m_current_limit{32000};
};

}  // namespace malama::ui

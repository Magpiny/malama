// /////////////////////////////////////////////////////////////////////////////
// Name:        include/ui/token_budget_bar.hpp
// Purpose:     Visual progress bar showing token budget utilization vs num_ctx
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <wx/panel.h>

namespace malama::ui {

class TokenBudgetBar final : public wxPanel {
   public:
    explicit TokenBudgetBar(wxWindow *parent_ptr);

    void UpdateUsage(std::size_t used_tokens, std::size_t max_tokens) noexcept;

   private:
    void on_paint(wxPaintEvent &event) noexcept;

    std::size_t m_used_tokens{0UZ};
    std::size_t m_max_tokens{4096UZ};
};

}  // namespace malama::ui

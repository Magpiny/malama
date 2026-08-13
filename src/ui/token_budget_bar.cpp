// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/token_budget_bar.cpp
// Purpose:     Custom double-buffered paint rendering for context token gauge
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/token_budget_bar.hpp"

#include <algorithm>
#include <format>
#include <memory>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace malama::ui {

inline constexpr int default_bar_height = 18;
inline constexpr double warning_threshold_ratio = 0.80;
inline constexpr double danger_threshold_ratio = 0.95;

TokenBudgetBar::TokenBudgetBar(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY, wxDefaultPosition, wxSize(-1, default_bar_height),
              wxFULL_REPAINT_ON_RESIZE) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &TokenBudgetBar::on_paint, this);
}

void TokenBudgetBar::UpdateUsage(std::size_t used_tokens, std::size_t max_tokens) noexcept {
    m_used_tokens = used_tokens;
    m_max_tokens = (max_tokens == 0UZ) ? 4096UZ : max_tokens;
    Refresh(false);
    Update();
}

void TokenBudgetBar::on_paint(wxPaintEvent &WXUNUSED(event)) noexcept {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(wxColour("#1A0105")));
    dc.Clear();

    const wxSize client_size = GetClientSize();
    if (client_size.GetWidth() <= 0 || client_size.GetHeight() <= 0) [[unlikely]] {
        return;
    }

    auto gc_ptr = std::unique_ptr<wxGraphicsContext>(wxGraphicsContext::Create(dc));
    if (gc_ptr == nullptr) [[unlikely]] {
        return;
    }

    const auto width = static_cast<double>(client_size.GetWidth());
    const auto height = static_cast<double>(client_size.GetHeight());
    const double ratio =
        std::min(1.0, static_cast<double>(m_used_tokens) / static_cast<double>(m_max_tokens));

    // Render Track Background
    gc_ptr->SetBrush(wxBrush(wxColour("#2D2D2D")));
    gc_ptr->SetPen(*wxTRANSPARENT_PEN);
    gc_ptr->DrawRoundedRectangle(0.0, 0.0, width, height, 4.0);

    // Pick Bar Color based on utilization threshold
    wxColour fill_color("#4CAF50");  // Normal: Green
    if (ratio >= danger_threshold_ratio) {
        fill_color = wxColour("#E53935");  // Danger: Red
    } else if (ratio >= warning_threshold_ratio) {
        fill_color = wxColour("#FB8C00");  // Warning: Amber
    }

    // Render Filled Progress Track
    if (ratio > 0.0) {
        gc_ptr->SetBrush(wxBrush(fill_color));
        gc_ptr->DrawRoundedRectangle(0.0, 0.0, width * ratio, height, 4.0);
    }

    // Overlay Context Usage Text Label
    wxFont font(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    dc.SetFont(font);
    dc.SetTextForeground(wxColour("#F5F5F7"));

    const std::string label = std::format("Context Budget: {} / {} tokens ({:.1f}%)", m_used_tokens,
                                          m_max_tokens, ratio * 100.0);
    const wxSize text_size = dc.GetTextExtent(wxString::FromUTF8(label));
    const int text_x = (client_size.GetWidth() - text_size.GetWidth()) / 2;
    const int text_y = (client_size.GetHeight() - text_size.GetHeight()) / 2;

    dc.DrawText(wxString::FromUTF8(label), text_x, text_y);
}

}  // namespace malama::ui

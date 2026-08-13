// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/context_bar.cpp
// Purpose:     Implementation of pre-flight context budget estimator widget
// Author:      Wanjare <samuelwanjare@protonmail.com>
// Created:     2026-08-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-later

#include "ui/context_bar.hpp"

#include <algorithm>
#include <format>
#include <wx/colour.h>
#include <wx/sizer.h>

namespace malama::ui {

ContextBar::ContextBar(wxWindow *parent, wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL) {
    InitializeComponents();
}

void ContextBar::InitializeComponents() noexcept {
    auto *main_sizer = new wxBoxSizer(wxVERTICAL);

    // Header label & status readout
    auto *header_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto *title = new wxStaticText(this, wxID_ANY, "Pre-Flight Context Budget:");
    title->SetFont(title->GetFont().Bold());

    m_status_label = new wxStaticText(this, wxID_ANY, "0 / 32,000 tokens (0.0%)");
    header_sizer->Add(title, 0, wxRIGHT, 10);
    header_sizer->Add(m_status_label, 1, wxEXPAND);

    main_sizer->Add(header_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);

    // Progress Gauge Bar
    m_gauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 12), wxGA_HORIZONTAL);
    main_sizer->Add(m_gauge, 0, wxEXPAND | wxALL, 6);

    // Warning Banner (Hidden when usage is within limits)
    m_warning_label = new wxStaticText(this, wxID_ANY, "");
    m_warning_label->SetForegroundColour(wxColour(220, 50, 50));
    m_warning_label->SetFont(m_warning_label->GetFont().Bold());
    m_warning_label->Hide();

    main_sizer->Add(m_warning_label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    SetSizer(main_sizer);
}

void ContextBar::UpdateBudget(std::size_t prompt_tokens, std::size_t attachment_tokens,
                              std::size_t max_context_limit) noexcept {
    if (max_context_limit == 0) {
        max_context_limit = 32000;
    }
    m_current_limit = max_context_limit;

    const std::size_t total_tokens = prompt_tokens + attachment_tokens;
    const double usage_percent =
        (static_cast<double>(total_tokens) / static_cast<double>(max_context_limit)) * 100.0;

    m_status_label->SetLabel(std::format("{} / {} tokens ({:.1f}%) — Prompt: {}, Attachments: {}",
                                         total_tokens, max_context_limit, usage_percent,
                                         prompt_tokens, attachment_tokens));

    const int gauge_val = std::min(100, static_cast<int>(usage_percent));
    m_gauge->SetValue(gauge_val);

    if (total_tokens > max_context_limit) {
        m_warning_label->SetLabel(
            std::format("⚠️ CONTEXT OVERFLOW: Payload exceeds boundary by {} tokens! Input will be "
                        "truncated by Ollama.",
                        total_tokens - max_context_limit));
        if (m_warning_label->IsShown() == false) {
            m_warning_label->Show();
            Layout();
        }
    } else if (usage_percent >= 85.0) {
        m_warning_label->SetLabel(std::format(
            "⚠️ High Context Load ({:.1f}%): Approaching maximum capacity limit.", usage_percent));
        m_warning_label->SetForegroundColour(wxColour(210, 130, 20));
        if (m_warning_label->IsShown() == false) {
            m_warning_label->Show();
            Layout();
        }
    } else {
        if (m_warning_label->IsShown() == true) {
            m_warning_label->Hide();
            Layout();
        }
    }
}

}  // namespace malama::ui

// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/session_params_dialog.cpp
// Purpose:     Implementation of modal dialog for per-session parameter tuning
// Author:      Wanjare <samuelwanjare@protonmail.com>
// Created:     2026-08-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-later

#include "ui/session_params_dlg.hpp"

#include <algorithm>
#include <format>
#include <utility>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbox.h>

namespace malama::ui {

namespace {

constexpr int kSliderTempScale = 100;
constexpr uint32_t kMinCtx = 2048;
constexpr uint32_t kMaxCtx = 128000;

constexpr uint16_t session_params_dlg_width = 600;
constexpr uint16_t session_params_dlg_height = 680;

}  // namespace

SessionParamsDialog::SessionParamsDialog(wxWindow *parent, common::SessionParameters params)
    : wxDialog(parent, wxID_ANY, "Session Parameter Tuning & System Prompt", wxDefaultPosition,
               wxSize(session_params_dlg_width, session_params_dlg_height),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_params(std::move(params)) {
    InitializeComponents();
    BindEvents();
    CenterOnParent();
}

void SessionParamsDialog::InitializeComponents() noexcept {
    auto *main_sizer = new wxBoxSizer(wxVERTICAL);

    // 1. Model Inference Parameters Section
    auto *params_box = new wxStaticBoxSizer(wxVERTICAL, this, "Inference Parameters");
    auto *grid_sizer = new wxFlexGridSizer(5, 3, 10, 10);
    grid_sizer->AddGrowableCol(1, 1);

    // --- Temperature Slider ---
    grid_sizer->Add(new wxStaticText(this, wxID_ANY, "Temperature:"), 0, wxALIGN_CENTER_VERTICAL);
    const int initial_temp_idx =
        static_cast<int>(m_params.m_temperature * static_cast<float>(kSliderTempScale));
    m_temp_slider = new wxSlider(this, wxID_ANY, initial_temp_idx, 0, 200, wxDefaultPosition,
                                 wxDefaultSize, wxSL_HORIZONTAL);
    grid_sizer->Add(m_temp_slider, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    m_temp_value_label =
        new wxStaticText(this, wxID_ANY, std::format("{:.2f}", m_params.m_temperature),
                         wxDefaultPosition, wxSize(50, -1));
    grid_sizer->Add(m_temp_value_label, 0, wxALIGN_CENTER_VERTICAL);

    // --- Context Capacity (num_ctx) Slider ---
    grid_sizer->Add(new wxStaticText(this, wxID_ANY, "Context Size (num_ctx):"), 0,
                    wxALIGN_CENTER_VERTICAL);
    m_num_ctx_slider = new wxSlider(this, wxID_ANY, static_cast<int>(m_params.m_num_ctx),
                                    static_cast<int>(kMinCtx), static_cast<int>(kMaxCtx),
                                    wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
    grid_sizer->Add(m_num_ctx_slider, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    m_num_ctx_value_label =
        new wxStaticText(this, wxID_ANY, std::format("{} tokens", m_params.m_num_ctx),
                         wxDefaultPosition, wxSize(80, -1));
    grid_sizer->Add(m_num_ctx_value_label, 0, wxALIGN_CENTER_VERTICAL);

    // --- Top-P Control ---
    grid_sizer->Add(new wxStaticText(this, wxID_ANY, "Top P (0.0 - 1.0):"), 0,
                    wxALIGN_CENTER_VERTICAL);
    m_top_p_ctrl = new wxTextCtrl(this, wxID_ANY, std::format("{:.2f}", m_params.m_top_p));
    grid_sizer->Add(m_top_p_ctrl, 0, wxEXPAND);
    grid_sizer->AddStretchSpacer();

    // --- Top-K Control ---
    grid_sizer->Add(new wxStaticText(this, wxID_ANY, "Top K:"), 0, wxALIGN_CENTER_VERTICAL);
    m_top_k_ctrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                  wxSP_ARROW_KEYS, 0, 200, m_params.m_top_k);
    grid_sizer->Add(m_top_k_ctrl, 0, wxEXPAND);
    grid_sizer->AddStretchSpacer();

    // --- Repeat Penalty Control ---
    grid_sizer->Add(new wxStaticText(this, wxID_ANY, "Repeat Penalty:"), 0,
                    wxALIGN_CENTER_VERTICAL);
    m_repeat_penalty_ctrl =
        new wxTextCtrl(this, wxID_ANY, std::format("{:.2f}", m_params.m_repeat_penalty));
    grid_sizer->Add(m_repeat_penalty_ctrl, 0, wxEXPAND);
    grid_sizer->AddStretchSpacer();

    params_box->Add(grid_sizer, 1, wxEXPAND | wxALL, 8);
    main_sizer->Add(params_box, 0, wxEXPAND | wxALL, 10);

    // 2. Custom System Prompt Section
    auto *prompt_box = new wxStaticBoxSizer(wxVERTICAL, this, "System Instructions");
    prompt_box->Add(
        new wxStaticText(this, wxID_ANY,
                         "Inject custom behavior or context rules for this conversation:"),
        0, wxBOTTOM, 5);

    m_system_prompt_ctrl =
        new wxTextCtrl(this, wxID_ANY, m_params.m_system_prompt, wxDefaultPosition, wxSize(-1, 150),
                       wxTE_MULTILINE | wxHSCROLL);
    prompt_box->Add(m_system_prompt_ctrl, 1, wxEXPAND | wxALL, 5);
    main_sizer->Add(prompt_box, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    // 3. Action Buttons Sizer
    auto *button_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto *reset_btn = new wxButton(this, wxID_ANY, "Reset Defaults");
    button_sizer->Add(reset_btn, 0, wxALIGN_CENTER_VERTICAL);
    button_sizer->AddStretchSpacer();

    auto *std_buttons = CreateButtonSizer(wxOK | wxCANCEL);
    if (std_buttons != nullptr) {
        button_sizer->Add(std_buttons, 0, wxALIGN_CENTER_VERTICAL);
    }

    main_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 10);
    SetSizer(main_sizer);

    reset_btn->Bind(wxEVT_BUTTON, &SessionParamsDialog::OnResetDefaults, this);
}

void SessionParamsDialog::BindEvents() noexcept {
    m_temp_slider->Bind(wxEVT_SLIDER, &SessionParamsDialog::OnTemperatureSlider, this);
    m_num_ctx_slider->Bind(wxEVT_SLIDER, &SessionParamsDialog::OnNumCtxSlider, this);
}

void SessionParamsDialog::OnTemperatureSlider(wxCommandEvent & /*event*/) noexcept {
    const float val =
        static_cast<float>(m_temp_slider->GetValue()) / static_cast<float>(kSliderTempScale);
    m_temp_value_label->SetLabel(std::format("{:.2f}", val));
}

void SessionParamsDialog::OnNumCtxSlider(wxCommandEvent & /*event*/) noexcept {
    const int val = m_num_ctx_slider->GetValue();
    m_num_ctx_value_label->SetLabel(std::format("{} tokens", val));
}

void SessionParamsDialog::OnResetDefaults(wxCommandEvent & /*event*/) noexcept {
    const common::SessionParameters defaults{};
    m_temp_slider->SetValue(
        static_cast<int>(defaults.m_temperature * static_cast<float>(kSliderTempScale)));
    m_temp_value_label->SetLabel(std::format("{:.2f}", defaults.m_temperature));

    m_num_ctx_slider->SetValue(static_cast<int>(defaults.m_num_ctx));
    m_num_ctx_value_label->SetLabel(std::format("{} tokens", defaults.m_num_ctx));

    m_top_p_ctrl->SetValue(std::format("{:.2f}", defaults.m_top_p));
    m_top_k_ctrl->SetValue(defaults.m_top_k);
    m_repeat_penalty_ctrl->SetValue(std::format("{:.2f}", defaults.m_repeat_penalty));
    m_system_prompt_ctrl->SetValue(defaults.m_system_prompt);
}

[[nodiscard]] common::SessionParameters SessionParamsDialog::GetParameters() const noexcept {
    common::SessionParameters result;
    result.m_temperature =
        static_cast<float>(m_temp_slider->GetValue()) / static_cast<float>(kSliderTempScale);
    result.m_num_ctx = static_cast<uint32_t>(m_num_ctx_slider->GetValue());

    try {
        result.m_top_p = std::stof(m_top_p_ctrl->GetValue().ToStdString());
    } catch (...) {
        result.m_top_p = constants::top_p;
    }

    result.m_top_k = m_top_k_ctrl->GetValue();

    try {
        result.m_repeat_penalty = std::stof(m_repeat_penalty_ctrl->GetValue().ToStdString());
    } catch (...) {
        result.m_repeat_penalty = constants::rpt_penalty;
    }

    result.m_system_prompt = m_system_prompt_ctrl->GetValue().ToStdString();
    return result;
}

}  // namespace malama::ui

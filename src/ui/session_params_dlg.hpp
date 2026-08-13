// /////////////////////////////////////////////////////////////////////////////
// Name:        include/ui/session_params_dialog.hpp
// Purpose:     Modal dialog header for per-session parameter tuning and system prompt overrides
// Author:      Wanjare <samuelwanjare@protonmail.com>
// Created:     2026-08-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-later

#include <wx/dialog.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "common/types.hpp"

namespace malama::ui {

/**
 * @class SessionParamsDialog
 * @brief Modal dialog offering parameter controls (temperature, num_ctx, top_p, etc.) and custom
 * system instructions.
 */
class SessionParamsDialog final : public wxDialog {
   public:
    /**
     * @brief Construct the parameter tuning dialog.
     * @param parent Pointer to parent window.
     * @param params Current parameter settings to pre-populate.
     */
    explicit SessionParamsDialog(wxWindow *parent, common::SessionParameters params);

    ~SessionParamsDialog() override = default;

    SessionParamsDialog(const SessionParamsDialog &) = delete;
    SessionParamsDialog &operator=(const SessionParamsDialog &) = delete;
    SessionParamsDialog(SessionParamsDialog &&) = delete;
    SessionParamsDialog &operator=(SessionParamsDialog &&) = delete;

    /**
     * @brief Retrieve updated session parameters after OK selection.
     * @return Updated SessionParameters object.
     */
    [[nodiscard]] common::SessionParameters GetParameters() const noexcept;

   private:
    void InitializeComponents() noexcept;
    void BindEvents() noexcept;

    void OnTemperatureSlider(wxCommandEvent &event) noexcept;
    void OnNumCtxSlider(wxCommandEvent &event) noexcept;
    void OnResetDefaults(wxCommandEvent &event) noexcept;

    common::SessionParameters m_params;

    wxSlider *m_temp_slider{nullptr};
    wxStaticText *m_temp_value_label{nullptr};

    wxSlider *m_num_ctx_slider{nullptr};
    wxStaticText *m_num_ctx_value_label{nullptr};

    wxTextCtrl *m_top_p_ctrl{nullptr};
    wxSpinCtrl *m_top_k_ctrl{nullptr};
    wxTextCtrl *m_repeat_penalty_ctrl{nullptr};

    wxTextCtrl *m_system_prompt_ctrl{nullptr};
};

}  // namespace malama::ui

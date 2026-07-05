// /////////////////////////////////////////////////////////////////////////////
// Name:        include/ui/settings_dialog.hpp
// Purpose:     Preferences modal layout configuration for dynamic engines
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/clrpicker.h>
#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "config/config_manager.hpp"

namespace malama::ui {

class SettingsDialog : public wxDialog {
   public:
    explicit SettingsDialog(wxWindow *parent_ptr);
    ~SettingsDialog() override = default;

    SettingsDialog(const SettingsDialog &) = delete;
    auto operator=(const SettingsDialog &) -> SettingsDialog & = delete;
    SettingsDialog(SettingsDialog &&) noexcept = delete;
    auto operator=(SettingsDialog &&) noexcept -> SettingsDialog & = delete;

   private:
    auto setup_layout() noexcept -> void;
    auto populate_data() noexcept -> void;
    auto save_data() noexcept -> void;
    void on_save([[maybe_unused]] wxCommandEvent &event) noexcept;
    void on_refresh_models([[maybe_unused]] wxCommandEvent &event) noexcept;

    [[nodiscard]] auto fetch_local_models(const std::string &host, const std::string &port) noexcept
        -> std::vector<std::string>;

    wxNotebook *m_notebook_ptr{nullptr};

    // Engine Tab controls
    wxTextCtrl *m_host_input_ptr{nullptr};
    wxTextCtrl *m_port_input_ptr{nullptr};
    wxChoice *m_model_choice_ptr{nullptr};
    wxButton *m_refresh_btn_ptr{nullptr};
    wxStaticText *m_status_label_ptr{nullptr};
    wxCheckBox *m_thinking_check_ptr{nullptr};

    // Interaction Tab controls
    wxSpinCtrl *m_delay_spin_ptr{nullptr};

    // Appearance Tab Controls (Upgraded v0.2.6 Matrix)
    wxColourPickerCtrl *m_bg_picker_ptr{nullptr};
    wxColourPickerCtrl *m_surface_picker_ptr{nullptr};
    wxColourPickerCtrl *m_text_primary_picker_ptr{nullptr};
    wxColourPickerCtrl *m_text_accent_picker_ptr{nullptr};
    wxColourPickerCtrl *m_code_bg_picker_ptr{nullptr};

    // Sidebar Customization Controls
    wxColourPickerCtrl *m_sidebar_bg_picker_ptr{nullptr};
    wxColourPickerCtrl *m_sidebar_text_picker_ptr{nullptr};

    // Font Typography Customization Controls
    wxChoice *m_font_family_choice_ptr{nullptr};
    wxSpinCtrl *m_font_size_spin_ptr{nullptr};

    config::AppConfig m_local_config;
};

}  // namespace malama::ui

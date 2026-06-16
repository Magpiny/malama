#pragma once

#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include "config/config_manager.hpp"

namespace malama::ui {

class SettingsDialog : public wxDialog {
public:
    explicit SettingsDialog(wxWindow* parent_ptr);
    ~SettingsDialog() override = default;

private:
    auto setup_layout() noexcept -> void;
    auto populate_data() noexcept -> void;
    auto save_data() noexcept -> void;
    void on_save([[maybe_unused]] wxCommandEvent& event) noexcept;

    wxNotebook* m_notebook_ptr{nullptr};
    
    // Engine Tab
    wxTextCtrl* m_host_input_ptr{nullptr};
    wxTextCtrl* m_port_input_ptr{nullptr};
    wxTextCtrl* m_model_input_ptr{nullptr};

    // Interaction Tab
    wxSpinCtrl* m_delay_spin_ptr{nullptr};

    config::AppConfig m_local_config;
};

} // namespace malama::ui

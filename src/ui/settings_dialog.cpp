#include "ui/settings_dialog.hpp"
#include "core/owner.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/panel.h>

namespace malama::ui {

SettingsDialog::SettingsDialog(wxWindow* parent_ptr)
    : wxDialog(parent_ptr, wxID_ANY, "malama Preferences", wxDefaultPosition, wxSize(450, 400)) {
    m_local_config = config::ConfigManager::get_instance().get_config();
    setup_layout();
    populate_data();
}

auto SettingsDialog::setup_layout() noexcept -> void {
    malama::owner<wxBoxSizer*> main_sizer = new (std::nothrow) wxBoxSizer(wxVERTICAL);
    m_notebook_ptr = new (std::nothrow) wxNotebook(this, wxID_ANY);

    // --- ENGINE TAB ---
    auto *engine_panel = new (std::nothrow) wxPanel(m_notebook_ptr);
    auto *engine_sizer = new (std::nothrow) wxFlexGridSizer(2, 10, 10);
    engine_sizer->AddGrowableCol(1, 1);

    m_host_input_ptr = new (std::nothrow) wxTextCtrl(engine_panel, wxID_ANY);
    m_port_input_ptr = new (std::nothrow) wxTextCtrl(engine_panel, wxID_ANY);
    m_model_input_ptr = new (std::nothrow) wxTextCtrl(engine_panel, wxID_ANY);

    engine_sizer->Add(new wxStaticText(engine_panel, wxID_ANY, "Ollama Host:"), 0, wxALIGN_CENTER_VERTICAL);
    engine_sizer->Add(m_host_input_ptr, 1, wxEXPAND);
    engine_sizer->Add(new wxStaticText(engine_panel, wxID_ANY, "Ollama Port:"), 0, wxALIGN_CENTER_VERTICAL);
    engine_sizer->Add(m_port_input_ptr, 1, wxEXPAND);
    engine_sizer->Add(new wxStaticText(engine_panel, wxID_ANY, "Default Model:"), 0, wxALIGN_CENTER_VERTICAL);
    engine_sizer->Add(m_model_input_ptr, 1, wxEXPAND);

    engine_panel->SetSizer(engine_sizer);
    m_notebook_ptr->AddPage(engine_panel, "Engine", true);

    // --- INTERACTION TAB ---
    malama::owner<wxPanel*> interaction_panel = new (std::nothrow) wxPanel(m_notebook_ptr);
    auto *interact_sizer = new (std::nothrow) wxFlexGridSizer(2, 10, 10);
    
    m_delay_spin_ptr = new (std::nothrow) wxSpinCtrl(interaction_panel, wxID_ANY);
    m_delay_spin_ptr->SetRange(0, 100);

    interact_sizer->Add(new wxStaticText(interaction_panel, wxID_ANY, "Typewriter Delay (ms):"), 0, wxALIGN_CENTER_VERTICAL);
    interact_sizer->Add(m_delay_spin_ptr, 0, wxEXPAND);

    interaction_panel->SetSizer(interact_sizer);
    m_notebook_ptr->AddPage(interaction_panel, "Interaction");

    // --- BUTTONS ---
    auto *btn_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);
    malama::owner<wxButton*> save_btn = new (std::nothrow) wxButton(this, wxID_ANY, "Save Settings");
    save_btn->Bind(wxEVT_BUTTON, &SettingsDialog::on_save, this);
    
    btn_sizer->AddStretchSpacer();
    btn_sizer->Add(save_btn, 0, wxALL, 10);

    main_sizer->Add(m_notebook_ptr, 1, wxEXPAND | wxALL, 10);
    main_sizer->Add(btn_sizer, 0, wxEXPAND);

    SetSizer(main_sizer);
}

auto SettingsDialog::populate_data() noexcept -> void {
    m_host_input_ptr->SetValue(m_local_config.m_engine.m_host);
    m_port_input_ptr->SetValue(m_local_config.m_engine.m_port);
    m_model_input_ptr->SetValue(m_local_config.m_engine.m_active_model);
    m_delay_spin_ptr->SetValue(m_local_config.m_interaction.m_typewriter_delay_ms);
}

auto SettingsDialog::save_data() noexcept -> void {
    m_local_config.m_engine.m_host = m_host_input_ptr->GetValue().ToStdString();
    m_local_config.m_engine.m_port = m_port_input_ptr->GetValue().ToStdString();
    m_local_config.m_engine.m_active_model = m_model_input_ptr->GetValue().ToStdString();
    m_local_config.m_interaction.m_typewriter_delay_ms = m_delay_spin_ptr->GetValue();

    auto& confmgr = config::ConfigManager::get_instance();
    confmgr.update_config(m_local_config);
    confmgr.save_config();
}

void SettingsDialog::on_save([[maybe_unused]] wxCommandEvent& event) noexcept {
    save_data();
    EndModal(wxID_OK);
}

} // namespace malama::ui

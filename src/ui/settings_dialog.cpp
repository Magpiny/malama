// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/settings_dialog.cpp
// Purpose:     Implements preferences configurations with dynamic engine checks
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/settings_dialog.hpp"
#include "core/owner.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <boost/asio.hpp>
#include <glaze/glaze.hpp>
#include <new>
#include <algorithm>
#include <thread>
#include <cstdlib>

namespace malama::network {

struct OllamaModelItem final {
    std::string name{};
};

struct OllamaTagsResponse final {
    std::vector<OllamaModelItem> models{};
};

} // namespace malama::network

template <>
struct glz::meta<malama::network::OllamaModelItem> {
    using T = malama::network::OllamaModelItem;
    static constexpr auto value = object("name", &T::name);
};

template <>
struct glz::meta<malama::network::OllamaTagsResponse> {
    using T = malama::network::OllamaTagsResponse;
    static constexpr auto value = object("models", &T::models);
};

namespace malama::ui {

SettingsDialog::SettingsDialog(wxWindow* parent_ptr)
    : wxDialog(parent_ptr, wxID_ANY, "malama Preferences", wxDefaultPosition, wxSize(450, 460)) {
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
    
    auto *model_box_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);
    m_model_choice_ptr = new (std::nothrow) wxChoice(engine_panel, wxID_ANY);
    m_refresh_btn_ptr = new (std::nothrow) wxButton(engine_panel, wxID_ANY, "Refresh");
    m_refresh_btn_ptr->Bind(wxEVT_BUTTON, &SettingsDialog::on_refresh_models, this);
    
    model_box_sizer->Add(m_model_choice_ptr, 1, wxEXPAND | wxRIGHT, 5);
    model_box_sizer->Add(m_refresh_btn_ptr, 0, wxBU_EXACTFIT);

    m_status_label_ptr = new (std::nothrow) wxStaticText(engine_panel, wxID_ANY, "Checking...");
    m_thinking_check_ptr = new (std::nothrow) wxCheckBox(
        engine_panel, wxID_ANY, "Enable Model Reasoning Output (<think> tags)"
    );

    engine_sizer->Add(
        new wxStaticText(engine_panel, wxID_ANY, "Ollama Host:"), 0, wxALIGN_CENTER_VERTICAL
    );
    engine_sizer->Add(m_host_input_ptr, 1, wxEXPAND);
    
    engine_sizer->Add(
        new wxStaticText(engine_panel, wxID_ANY, "Ollama Port:"), 0, wxALIGN_CENTER_VERTICAL
    );
    engine_sizer->Add(m_port_input_ptr, 1, wxEXPAND);
    
    engine_sizer->Add(
        new wxStaticText(engine_panel, wxID_ANY, "Default Model:"), 0, wxALIGN_CENTER_VERTICAL
    );
    engine_sizer->Add(model_box_sizer, 1, wxEXPAND);
    
    engine_sizer->Add(
        new wxStaticText(engine_panel, wxID_ANY, "Engine Status:"), 0, wxALIGN_CENTER_VERTICAL
    );
    engine_sizer->Add(m_status_label_ptr, 1, wxALIGN_CENTER_VERTICAL);

    engine_sizer->Add(
        new wxStaticText(engine_panel, wxID_ANY, "Model Parameters:"), 0, wxALIGN_CENTER_VERTICAL
    );
    engine_sizer->Add(m_thinking_check_ptr, 1, wxEXPAND);

    engine_panel->SetSizer(engine_sizer);
    m_notebook_ptr->AddPage(engine_panel, "Engine", true);

    // --- INTERACTION TAB ---
    malama::owner<wxPanel*> interaction_panel = new (std::nothrow) wxPanel(m_notebook_ptr);
    auto *interact_sizer = new (std::nothrow) wxFlexGridSizer(2, 10, 10);
    
    m_delay_spin_ptr = new (std::nothrow) wxSpinCtrl(interaction_panel, wxID_ANY);
    m_delay_spin_ptr->SetRange(0, 100);

    interact_sizer->Add(
        new wxStaticText(interaction_panel, wxID_ANY, "Typewriter Delay (ms):"), 
        0, wxALIGN_CENTER_VERTICAL
    );
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
    m_thinking_check_ptr->SetValue(m_local_config.m_engine.m_thinking_enabled);

    std::string active_host = m_local_config.m_engine.m_host;
    std::string active_port = m_local_config.m_engine.m_port;
    auto available_models = fetch_local_models(active_host, active_port);

    m_model_choice_ptr->Clear();
    if (available_models.empty()) {
        m_status_label_ptr->SetLabel("Ollama Not Detected (Offline)");
        m_status_label_ptr->SetForegroundColour(wxColour(250, 100, 100));
        m_model_choice_ptr->Append(wxString::FromUTF8(m_local_config.m_engine.m_active_model));
        m_model_choice_ptr->SetSelection(0);
    } else {
        m_status_label_ptr->SetLabel("Ollama Running (Online)");
        m_status_label_ptr->SetForegroundColour(wxColour(100, 220, 100));
        
        int selected_index = 0;
        for (int index = 0; index < static_cast<int>(available_models.size()); ++index) {
            m_model_choice_ptr->Append(wxString::FromUTF8(available_models[index]));
            if (available_models[index] == m_local_config.m_engine.m_active_model) {
                selected_index = index;
            }
        }
        m_model_choice_ptr->SetSelection(selected_index);
    }
    m_delay_spin_ptr->SetValue(m_local_config.m_interaction.m_typewriter_delay_ms);
}

auto SettingsDialog::fetch_local_models(
    const std::string& host, 
    const std::string& port
) noexcept -> std::vector<std::string> {
    std::vector<std::string> models_list{};
    try {
        boost::asio::io_context io_ctx;
        boost::asio::ip::tcp::resolver resolver(io_ctx);
        auto endpoints = resolver.resolve(host, port);
        boost::asio::ip::tcp::socket socket(io_ctx);
        boost::asio::connect(socket, endpoints);

        std::string request = "GET /api/tags HTTP/1.0\r\nHost: " 
                            + host + ":" + port + "\r\n\r\n";
        boost::asio::write(socket, boost::asio::buffer(request));

        std::string response_str;
        char buffer[1024];
        boost::system::error_code err_code;
        
        while (std::size_t length = socket.read_some(boost::asio::buffer(buffer), err_code)) {
            response_str.append(buffer, length);
            if (err_code) {
                break;
            }
        }

        std::size_t body_pos = response_str.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            std::string body_content = response_str.substr(body_pos + 4);
            if (!body_content.empty() && body_content.front() == '{') {
                network::OllamaTagsResponse parsed_res{};
                const auto parse_err = glz::read<glz::opts{.error_on_unknown_keys = false}>(
                    parsed_res, body_content
                );
                if (!parse_err) {
                    for (const auto& item : parsed_res.models) {
                        if (!item.name.empty()) {
                            models_list.push_back(item.name);
                        }
                    }
                }
            }
        }
    } catch (...) {
        // Safe fallback handling for system offline states
    }
    return models_list;
}

auto SettingsDialog::save_data() noexcept -> void {
    auto& confmgr = config::ConfigManager::get_instance();
    const auto old_config = confmgr.get_config();

    m_local_config.m_engine.m_host = m_host_input_ptr->GetValue().ToStdString();
    m_local_config.m_engine.m_port = m_port_input_ptr->GetValue().ToStdString();
    m_local_config.m_engine.m_thinking_enabled = m_thinking_check_ptr->GetValue();
    
    int selection = m_model_choice_ptr->GetSelection();
    if (selection != wxNOT_FOUND) {
        m_local_config.m_engine.m_active_model = 
            m_model_choice_ptr->GetString(selection).ToStdString();
    }
    
    m_local_config.m_interaction.m_typewriter_delay_ms = m_delay_spin_ptr->GetValue();

    confmgr.update_config(m_local_config);
    confmgr.save_config();

    // Trigger background service reset only if model parameter values shifted
    if (old_config.m_engine.m_active_model != m_local_config.m_engine.m_active_model) {
        std::jthread restart_worker([]() {
            [[maybe_unused]] int status = std::system(
                "systemctl restart ollama 2>/dev/null || systemctl --user restart ollama 2>/dev/null"
            );
        });
        restart_worker.detach();
    }
}

void SettingsDialog::on_save([[maybe_unused]] wxCommandEvent& event) noexcept {
    save_data();
    EndModal(wxID_OK);
}

void SettingsDialog::on_refresh_models([[maybe_unused]] wxCommandEvent& event) noexcept {
    // Sync screen configuration states before executing populate passes
    m_local_config.m_engine.m_host = m_host_input_ptr->GetValue().ToStdString();
    m_local_config.m_engine.m_port = m_port_input_ptr->GetValue().ToStdString();
    m_local_config.m_engine.m_thinking_enabled = m_thinking_check_ptr->GetValue();
    
    int selection = m_model_choice_ptr->GetSelection();
    if (selection != wxNOT_FOUND) {
        m_local_config.m_engine.m_active_model = 
            m_model_choice_ptr->GetString(selection).ToStdString();
    }
    populate_data();
}

} // namespace malama::ui

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

#include <algorithm>
#include <boost/asio.hpp>
#include <cstdlib>
#include <glaze/glaze.hpp>
#include <new>
#include <thread>
#include <wx/button.h>
#include <wx/fontenum.h>  // Dynamic native system font discovery pipeline
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "spdlog/spdlog.h"

namespace malama::network {

struct OllamaModelItem final {
    std::string m_name{};
};

struct OllamaTagsResponse final {
    std::vector<OllamaModelItem> models{};
};

}  // namespace malama::network

template<>
struct glz::meta<malama::network::OllamaModelItem> {
    using T = malama::network::OllamaModelItem;
    static constexpr auto value = object("name", &T::m_name);
};

template<>
struct glz::meta<malama::network::OllamaTagsResponse> {
    using T = malama::network::OllamaTagsResponse;
    static constexpr auto value = object("models", &T::models);
};

namespace malama::ui {

// Named Constants to cleanly bypass tracking magic sizing attributes
namespace local_ui_constants {
constexpr int dialog_width = 500;
constexpr int dialog_height = 620;
constexpr int grid_columns = 2;
constexpr int layout_gap_v = 8;
constexpr int layout_gap_h = 8;
constexpr int notebook_padding = 10;
constexpr int component_spacing = 5;
constexpr int min_font_point = 8;
constexpr int max_font_point = 36;
}  // namespace local_ui_constants

SettingsDialog::SettingsDialog(wxWindow *parent_ptr)
    : wxDialog(parent_ptr, wxID_ANY, "malama Preferences", wxDefaultPosition,
               wxSize(local_ui_constants::dialog_width, local_ui_constants::dialog_height)) {
    m_local_config = config::ConfigManager::get_instance().get_config();
    setup_layout();
    populate_data();
}

auto SettingsDialog::setup_layout() noexcept -> void {
    // Allocation Exception Safeguard Pipeline
    auto *main_sizer = new (std::nothrow) wxBoxSizer(wxVERTICAL);
    m_notebook_ptr = new (std::nothrow) wxNotebook(this, wxID_ANY);

    if ((main_sizer == nullptr) || (m_notebook_ptr == nullptr)) {
        delete main_sizer;
        delete m_notebook_ptr;
        spdlog::error("Critical Memory Allocation Failure within Settings Dialog Layout Init.");
        return;
    }

    // =========================================================================
    // 1. ENGINE CONFIGURATION PANE
    // =========================================================================
    auto *engine_panel = new (std::nothrow) wxPanel(m_notebook_ptr);
    auto *engine_sizer = new (std::nothrow)
        wxFlexGridSizer(local_ui_constants::grid_columns, local_ui_constants::layout_gap_v,
                        local_ui_constants::layout_gap_h);

    if ((engine_panel == nullptr) || (engine_sizer == nullptr)) {
        delete engine_panel;
        delete engine_sizer;
        return;
    }
    engine_sizer->AddGrowableCol(1, 1);

    m_host_input_ptr = new (std::nothrow) wxTextCtrl(engine_panel, wxID_ANY);
    m_port_input_ptr = new (std::nothrow) wxTextCtrl(engine_panel, wxID_ANY);
    m_model_choice_ptr = new (std::nothrow) wxChoice(engine_panel, wxID_ANY);
    m_refresh_btn_ptr = new (std::nothrow) wxButton(engine_panel, wxID_ANY, "Refresh");
    m_status_label_ptr = new (std::nothrow) wxStaticText(engine_panel, wxID_ANY, "Checking...");
    m_thinking_check_ptr =
        new (std::nothrow) wxCheckBox(engine_panel, wxID_ANY, "Enable Reasoning");

    auto *model_box_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);

    if ((m_host_input_ptr == nullptr) || (m_port_input_ptr == nullptr) ||
        (m_model_choice_ptr == nullptr) || (m_refresh_btn_ptr == nullptr) ||
        (m_status_label_ptr == nullptr) || (m_thinking_check_ptr == nullptr) ||
        (model_box_sizer == nullptr)) {
        delete model_box_sizer;
        return;
    }

    m_refresh_btn_ptr->Bind(wxEVT_BUTTON, &SettingsDialog::on_refresh_models, this);
    model_box_sizer->Add(m_model_choice_ptr, 1, wxEXPAND | wxRIGHT,
                         local_ui_constants::component_spacing);
    model_box_sizer->Add(m_refresh_btn_ptr, 0, wxBU_EXACTFIT);

    engine_sizer->Add(new wxStaticText(engine_panel, wxID_ANY, "Ollama Host:"), 0,
                      wxALIGN_CENTER_VERTICAL);
    engine_sizer->Add(m_host_input_ptr, 1, wxEXPAND);
    engine_sizer->Add(new wxStaticText(engine_panel, wxID_ANY, "Ollama Port:"), 0,
                      wxALIGN_CENTER_VERTICAL);
    engine_sizer->Add(m_port_input_ptr, 1, wxEXPAND);
    engine_sizer->Add(new wxStaticText(engine_panel, wxID_ANY, "Default Model:"), 0,
                      wxALIGN_CENTER_VERTICAL);
    engine_sizer->Add(model_box_sizer, 1, wxEXPAND);
    engine_sizer->Add(new wxStaticText(engine_panel, wxID_ANY, "Engine Status:"), 0,
                      wxALIGN_CENTER_VERTICAL);
    engine_sizer->Add(m_status_label_ptr, 1, wxALIGN_CENTER_VERTICAL);
    engine_sizer->Add(new wxStaticText(engine_panel, wxID_ANY, "Reasoning Logic:"), 0,
                      wxALIGN_CENTER_VERTICAL);
    engine_sizer->Add(m_thinking_check_ptr, 1, wxEXPAND);

    engine_panel->SetSizer(engine_sizer);
    m_notebook_ptr->AddPage(engine_panel, "Engine", true);

    // =========================================================================
    // 2. INTERACTION CONFIGURATION PANE
    // =========================================================================
    auto *interaction_panel = new (std::nothrow) wxPanel(m_notebook_ptr);
    auto *interact_sizer = new (std::nothrow)
        wxFlexGridSizer(local_ui_constants::grid_columns, local_ui_constants::layout_gap_v,
                        local_ui_constants::layout_gap_h);
    m_delay_spin_ptr = new (std::nothrow) wxSpinCtrl(interaction_panel, wxID_ANY);

    if ((interaction_panel == nullptr) || (interact_sizer == nullptr) ||
        (m_delay_spin_ptr == nullptr)) {
        delete interaction_panel;
        delete interact_sizer;
        return;
    }
    m_delay_spin_ptr->SetRange(0, 100);
    interact_sizer->AddGrowableCol(1, 1);

    interact_sizer->Add(new wxStaticText(interaction_panel, wxID_ANY, "Typewriter Delay (ms):"), 0,
                        wxALIGN_CENTER_VERTICAL);
    interact_sizer->Add(m_delay_spin_ptr, 0, wxEXPAND);
    interaction_panel->SetSizer(interact_sizer);
    m_notebook_ptr->AddPage(interaction_panel, "Interaction");

    // =========================================================================
    // 3. EXTENDED CUSTOM APPEARANCE CONFIGURATION PANE (v0.2.6 Focus)
    // =========================================================================
    auto *appearance_panel = new (std::nothrow) wxPanel(m_notebook_ptr);
    auto *appearance_sizer = new (std::nothrow)
        wxFlexGridSizer(local_ui_constants::grid_columns, local_ui_constants::layout_gap_v,
                        local_ui_constants::layout_gap_h);

    if ((appearance_panel == nullptr) || (appearance_sizer == nullptr)) {
        delete appearance_panel;
        delete appearance_sizer;
        return;
    }
    appearance_sizer->AddGrowableCol(1, 1);

    // Instantiate System Custom Pickers via Safe Auto Syntax Passing
    m_bg_picker_ptr = new (std::nothrow) wxColourPickerCtrl(appearance_panel, wxID_ANY);
    m_surface_picker_ptr = new (std::nothrow) wxColourPickerCtrl(appearance_panel, wxID_ANY);
    m_text_primary_picker_ptr = new (std::nothrow) wxColourPickerCtrl(appearance_panel, wxID_ANY);
    m_text_accent_picker_ptr = new (std::nothrow) wxColourPickerCtrl(appearance_panel, wxID_ANY);
    m_code_bg_picker_ptr = new (std::nothrow) wxColourPickerCtrl(appearance_panel, wxID_ANY);
    m_sidebar_bg_picker_ptr = new (std::nothrow) wxColourPickerCtrl(appearance_panel, wxID_ANY);
    m_sidebar_text_picker_ptr = new (std::nothrow) wxColourPickerCtrl(appearance_panel, wxID_ANY);
    m_font_family_choice_ptr = new (std::nothrow) wxChoice(appearance_panel, wxID_ANY);
    m_font_size_spin_ptr = new (std::nothrow) wxSpinCtrl(appearance_panel, wxID_ANY);

    if ((m_bg_picker_ptr == nullptr) || (m_surface_picker_ptr == nullptr) ||
        (m_text_primary_picker_ptr == nullptr) || (m_text_accent_picker_ptr == nullptr) ||
        (m_code_bg_picker_ptr == nullptr) || (m_sidebar_bg_picker_ptr == nullptr) ||
        (m_sidebar_text_picker_ptr == nullptr) || (m_font_family_choice_ptr == nullptr) ||
        (m_font_size_spin_ptr == nullptr)) {
        return;
    }

    m_font_size_spin_ptr->SetRange(local_ui_constants::min_font_point,
                                   local_ui_constants::max_font_point);

    // Dynamic Discovery and Population of Host System Typography Components
    wxFontEnumerator font_enumerator;
    font_enumerator.EnumerateFacenames();
    auto installed_facenames = font_enumerator.GetFacenames();
    std::sort(installed_facenames.begin(), installed_facenames.end());

    for (const auto &family_name : installed_facenames) {
        m_font_family_choice_ptr->Append(family_name);
    }

    appearance_sizer->Add(new wxStaticText(appearance_panel, wxID_ANY, "App Font Style:"), 0,
                          wxALIGN_CENTER_VERTICAL);
    appearance_sizer->Add(m_font_family_choice_ptr, 0, wxEXPAND);
    appearance_sizer->Add(new wxStaticText(appearance_panel, wxID_ANY, "App Font Size (pt):"), 0,
                          wxALIGN_CENTER_VERTICAL);
    appearance_sizer->Add(m_font_size_spin_ptr, 0, wxEXPAND);

    appearance_sizer->Add(new wxStaticText(appearance_panel, wxID_ANY, "Main Background:"), 0,
                          wxALIGN_CENTER_VERTICAL);
    appearance_sizer->Add(m_bg_picker_ptr, 0, wxEXPAND);
    appearance_sizer->Add(new wxStaticText(appearance_panel, wxID_ANY, "Surface Color:"), 0,
                          wxALIGN_CENTER_VERTICAL);
    appearance_sizer->Add(m_surface_picker_ptr, 0, wxEXPAND);
    appearance_sizer->Add(new wxStaticText(appearance_panel, wxID_ANY, "Primary Typography:"), 0,
                          wxALIGN_CENTER_VERTICAL);
    appearance_sizer->Add(m_text_primary_picker_ptr, 0, wxEXPAND);
    appearance_sizer->Add(new wxStaticText(appearance_panel, wxID_ANY, "Accent Highlights:"), 0,
                          wxALIGN_CENTER_VERTICAL);
    appearance_sizer->Add(m_text_accent_picker_ptr, 0, wxEXPAND);

    appearance_sizer->Add(new wxStaticText(appearance_panel, wxID_ANY, "Sidebar Background:"), 0,
                          wxALIGN_CENTER_VERTICAL);
    appearance_sizer->Add(m_sidebar_bg_picker_ptr, 0, wxEXPAND);
    appearance_sizer->Add(new wxStaticText(appearance_panel, wxID_ANY, "Sidebar Text Color:"), 0,
                          wxALIGN_CENTER_VERTICAL);
    appearance_sizer->Add(m_sidebar_text_picker_ptr, 0, wxEXPAND);

    appearance_sizer->Add(new wxStaticText(appearance_panel, wxID_ANY, "Code Area Tint:"), 0,
                          wxALIGN_CENTER_VERTICAL);
    appearance_sizer->Add(m_code_bg_picker_ptr, 0, wxEXPAND);

    appearance_panel->SetSizer(appearance_sizer);
    m_notebook_ptr->AddPage(appearance_panel, "Appearance");

    // =========================================================================
    // 4. ACTION INTERFACE BUTTON RUNTIME CONTROL
    // =========================================================================
    auto *btn_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);
    auto *save_btn = new (std::nothrow) wxButton(this, wxID_ANY, "Save Settings");

    if ((btn_sizer == nullptr) || (save_btn == nullptr)) {
        delete btn_sizer;
        delete save_btn;
        return;
    }

    save_btn->Bind(wxEVT_BUTTON, &SettingsDialog::on_save, this);
    btn_sizer->AddStretchSpacer();
    btn_sizer->Add(save_btn, 0, wxALL, local_ui_constants::notebook_padding);

    main_sizer->Add(m_notebook_ptr, 1, wxEXPAND | wxALL, local_ui_constants::notebook_padding);
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
        for (std::size_t index = 0UZ; index < available_models.size(); ++index) {
            m_model_choice_ptr->Append(wxString::FromUTF8(available_models[index]));
            if (available_models[index] == m_local_config.m_engine.m_active_model) {
                selected_index = static_cast<int>(index);
            }
        }
        m_model_choice_ptr->SetSelection(selected_index);
    }
    m_delay_spin_ptr->SetValue(m_local_config.m_interaction.m_typewriter_delay_ms);

    // Load custom configuration keys securely into the interface elements
    m_bg_picker_ptr->SetColour(wxColour(m_local_config.m_appearance.m_bg_color));
    m_surface_picker_ptr->SetColour(wxColour(m_local_config.m_appearance.m_surface_color));
    m_text_primary_picker_ptr->SetColour(wxColour(m_local_config.m_appearance.m_text_primary));
    m_text_accent_picker_ptr->SetColour(wxColour(m_local_config.m_appearance.m_text_accent));
    m_code_bg_picker_ptr->SetColour(wxColour(m_local_config.m_appearance.m_code_bg));

    // Fallback binding for newly added sidebar parameters
    m_sidebar_bg_picker_ptr->SetColour(wxColour(m_local_config.m_appearance.m_sidebar_bg));
    m_sidebar_text_picker_ptr->SetColour(wxColour(m_local_config.m_appearance.m_sidebar_text));

    m_font_size_spin_ptr->SetValue(m_local_config.m_appearance.m_font_size);

    int matching_font_index = m_font_family_choice_ptr->FindString(
        wxString::FromUTF8(m_local_config.m_appearance.m_font_family));
    if (matching_font_index != wxNOT_FOUND) {
        m_font_family_choice_ptr->SetSelection(matching_font_index);
    }
}

auto SettingsDialog::fetch_local_models(const std::string &host, const std::string &port) noexcept
    -> std::vector<std::string> {
    std::vector<std::string> models_list{};
    try {
        boost::asio::io_context io_ctx;
        boost::asio::ip::tcp::resolver resolver(io_ctx);
        auto endpoints = resolver.resolve(host, port);
        boost::asio::ip::tcp::socket socket(io_ctx);
        boost::asio::connect(socket, endpoints);

        std::string request = "GET /api/tags HTTP/1.0\r\nHost: " + host + ":" + port + "\r\n\r\n";
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
                const auto parse_err =
                    glz::read<glz::opts{.error_on_unknown_keys = false}>(parsed_res, body_content);
                if (!parse_err) {
                    for (const auto &item : parsed_res.models) {
                        if (!item.m_name.empty()) {
                            models_list.push_back(item.m_name);
                        }
                    }
                }
            }
        }
    } catch (...) {
        // Safe exception tracking fallback handling for system offline states
    }
    return models_list;
}

auto SettingsDialog::save_data() noexcept -> void {
    auto &confmgr = config::ConfigManager::get_instance();
    const auto old_config = confmgr.get_config();

    m_local_config.m_engine.m_host = m_host_input_ptr->GetValue().ToStdString();
    m_local_config.m_engine.m_port = m_port_input_ptr->GetValue().ToStdString();
    m_local_config.m_engine.m_thinking_enabled = m_thinking_check_ptr->GetValue();

    int selection = m_model_choice_ptr->GetSelection();
    if (selection != wxNOT_FOUND) {
        m_local_config.m_engine.m_active_model =
            m_model_choice_ptr->GetString(static_cast<unsigned int>(selection)).ToStdString();
    }

    m_local_config.m_interaction.m_typewriter_delay_ms = m_delay_spin_ptr->GetValue();

    // Map the pickers values back to HTML hex strings
    m_local_config.m_appearance.m_bg_color =
        m_bg_picker_ptr->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
    m_local_config.m_appearance.m_surface_color =
        m_surface_picker_ptr->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
    m_local_config.m_appearance.m_text_primary =
        m_text_primary_picker_ptr->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
    m_local_config.m_appearance.m_text_accent =
        m_text_accent_picker_ptr->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
    m_local_config.m_appearance.m_code_bg =
        m_code_bg_picker_ptr->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();

    // Save Sidebar attributes
    m_local_config.m_appearance.m_sidebar_bg =
        m_sidebar_bg_picker_ptr->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
    m_local_config.m_appearance.m_sidebar_text =
        m_sidebar_text_picker_ptr->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();

    m_local_config.m_appearance.m_font_size = m_font_size_spin_ptr->GetValue();

    int font_selection = m_font_family_choice_ptr->GetSelection();
    if (font_selection != wxNOT_FOUND) {
        m_local_config.m_appearance.m_font_family =
            m_font_family_choice_ptr->GetString(static_cast<unsigned int>(font_selection))
                .ToStdString();
    }

    confmgr.update_config(m_local_config);
    confmgr.save_config();

    if (old_config.m_engine.m_active_model != m_local_config.m_engine.m_active_model) {
        std::jthread restart_worker([]() {
            [[maybe_unused]] int status = std::system(
                "systemctl restart ollama 2>/dev/null || systemctl --user restart ollama "
                "2>/dev/null");
        });
        restart_worker.detach();
    }
}

void SettingsDialog::on_save([[maybe_unused]] wxCommandEvent &event) noexcept {
    save_data();
    CallAfter([this]() { EndModal(wxID_OK); });
}

void SettingsDialog::on_refresh_models([[maybe_unused]] wxCommandEvent &event) noexcept {
    m_local_config.m_engine.m_host = m_host_input_ptr->GetValue().ToStdString();
    m_local_config.m_engine.m_port = m_port_input_ptr->GetValue().ToStdString();
    m_local_config.m_engine.m_thinking_enabled = m_thinking_check_ptr->GetValue();

    int selection = m_model_choice_ptr->GetSelection();
    if (selection != wxNOT_FOUND) {
        m_local_config.m_engine.m_active_model =
            m_model_choice_ptr->GetString(static_cast<unsigned int>(selection)).ToStdString();
    }
    populate_data();
}

}  // namespace malama::ui

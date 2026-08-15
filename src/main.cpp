/////////////////////////////////////////////////////////////////////////////
// Name:        src/main.cpp
// Purpose:     Main application entry point for malama native client
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>
#include <new>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <wx/image.h>
#include <wx/wx.h>

#include "common/constants.hpp"
#include "config/config_manager.hpp"
#include "network/ollama_client.hpp"
#include "network/stream_worker.hpp"
#include "ui/main_frame.hpp"

#if defined(__WXGTK__)
#include <glib.h>
#endif

namespace malama {

class MalamaApp final : public wxApp {
   public:
    explicit MalamaApp() = default;
    ~MalamaApp() override = default;

    MalamaApp(const MalamaApp &) = delete;
    auto operator=(const MalamaApp &) -> MalamaApp & = delete;
    MalamaApp(MalamaApp &&) noexcept = delete;
    auto operator=(MalamaApp &&) noexcept -> MalamaApp & = delete;

    [[nodiscard]] bool OnInit() override {
        wxInitAllImageHandlers();
        SetAppName("malama");
        SetAppDisplayName("Malama");

#if defined(__WXGTK__)
        // Binds Wayland app surface to malama-dev.desktop
        g_set_prgname("malama");
#endif

#if defined(__WXMSW__)
        wxIcon icon(wxT("malama.png"), wxBITMAP_TYPE_PNG);
        SetIcon(icon);
#endif

        spdlog::set_level(spdlog::level::debug);
        spdlog::info("Initializing malama v0.2.9 UI Customizations...\n");

        config::ConfigManager::get_instance().load_config("malama_config.json");
        const auto app_config = config::ConfigManager::get_instance().get_config();

        Bind(ui::EVT_MALAMA_TOKEN, &MalamaApp::OnTokenReceived, this);

        auto *raw_client_ptr = new (std::nothrow)
            network::OllamaClient(app_config.m_engine.m_host, app_config.m_engine.m_port);
        if (raw_client_ptr == nullptr) {
            return false;
        }
        std::unique_ptr<network::OllamaClient> client_ptr(raw_client_ptr);

        auto *raw_worker_ptr = new (std::nothrow) network::StreamWorker(std::move(client_ptr));
        if (raw_worker_ptr == nullptr) {
            return false;
        }

        // FIXED: Removed premature closing brace before this line
        m_worker_ptr.reset(raw_worker_ptr);

        auto *frame_ptr = new (std::nothrow) ui::MainFrame(
            "Malama", wxDefaultPosition,
            wxSize(constants::default_window_width, constants::default_window_height),
            [this](const std::string &user_prompt) mutable {
                auto *current_frame_ptr = dynamic_cast<ui::MainFrame *>(GetTopWindow());
                if (current_frame_ptr != nullptr) {
                    current_frame_ptr->AppendUserMessage(user_prompt);
                }

                if (m_worker_ptr != nullptr && current_frame_ptr != nullptr) {
                    const auto current_config = config::ConfigManager::get_instance().get_config();

                    auto full_history = current_frame_ptr->GetActiveSessionMessages();
                    std::vector<core::Message> history_without_current_turn;
                    if (!full_history.empty()) {
                        history_without_current_turn.assign(full_history.begin(),
                                                            full_history.end() - 1);
                    }

                    m_worker_ptr->InitializeGeneration(
                        current_config.m_engine.m_active_model, user_prompt,
                        history_without_current_turn,
                        [](std::string_view parsed_token, bool is_final) mutable {
                            auto *event_ptr =
                                new (std::nothrow) wxThreadEvent(ui::EVT_MALAMA_TOKEN);
                            if (event_ptr != nullptr) {
                                event_ptr->SetString(
                                    wxString::FromUTF8(parsed_token.data(), parsed_token.size()));
                                event_ptr->SetInt(is_final ? 1 : 0);
                                wxQueueEvent(wxTheApp, event_ptr);
                            }
                        });
                }
            });

        if (frame_ptr == nullptr) {
            return false;
        }

        frame_ptr->Show(true);
        return true;
    }

   private:
    void OnTokenReceived(wxThreadEvent &event) {
        auto *frame_ptr = dynamic_cast<ui::MainFrame *>(GetTopWindow());
        if (frame_ptr != nullptr) {
            bool is_final = event.GetInt() == 1;
            if (is_final) {
                frame_ptr->FinalizeAssistantResponse();
            } else {
                frame_ptr->AppendToken(event.GetString().ToStdString(wxConvUTF8));
            }
        }
    }

    std::unique_ptr<network::StreamWorker> m_worker_ptr;
};

}  // namespace malama

wxIMPLEMENT_APP(malama::MalamaApp);

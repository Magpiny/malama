// /////////////////////////////////////////////////////////////////////////////
// Name:        src/main.cpp
// Purpose:     Main application entry point for malama native client
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include <wx/wx.h>
#include <new>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include "common/constants.hpp"
#include "network/ollama_client.hpp"
#include "network/stream_worker.hpp"
#include "ui/main_frame.hpp"

namespace malama {

class MalamaApp : public wxApp {
public:
    explicit MalamaApp() = default;
    ~MalamaApp() override = default;

    MalamaApp(const MalamaApp &) = delete;
    MalamaApp &operator=(const MalamaApp &) = delete;
    MalamaApp(MalamaApp &&) noexcept = delete;
    MalamaApp &operator=(MalamaApp &&) noexcept = delete;

    [[nodiscard]] bool OnInit() override {
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("Initializing malama v0.1.0 MVP Interactivity Engine...\n");

        Bind(ui::EVT_MALAMA_TOKEN, &MalamaApp::OnTokenReceived, this);

        auto *raw_client_ptr = new (std::nothrow) network::OllamaClient(
            std::string(constants::default_ollama_host), 
            std::string(constants::default_ollama_port)
        );
        if (raw_client_ptr == nullptr) {return false;}
        std::unique_ptr<network::OllamaClient> client_ptr(raw_client_ptr);

        auto *raw_worker_ptr = new (std::nothrow) network::StreamWorker(std::move(client_ptr));
        if (raw_worker_ptr == nullptr) {return false;}
        m_worker_ptr.reset(raw_worker_ptr);

        auto *frame_ptr = new (std::nothrow) ui::MainFrame(
            "malama Local UI Engine",
            wxDefaultPosition, 
            wxSize(constants::default_window_width, constants::default_window_height),
            [this](const std::string& user_prompt) mutable {

                auto *current_frame_ptr = dynamic_cast<ui::MainFrame*>(GetTopWindow());
                if (current_frame_ptr != nullptr) {
                    current_frame_ptr->AppendUserMessage(user_prompt);
                }

                if (m_worker_ptr != nullptr) {
                    m_worker_ptr->InitializeGeneration(
                        constants::fallback_model_name,
                        user_prompt,
                        std::vector<common::Message>{},
                        [](std::string_view parsed_token) mutable {
                            auto *event_ptr = new (std::nothrow) wxThreadEvent(ui::EVT_MALAMA_TOKEN); // NOLINT
                            if (event_ptr != nullptr) {
                                event_ptr->SetString(wxString::FromUTF8(parsed_token.data(), parsed_token.size()));
                                wxQueueEvent(wxTheApp, event_ptr);
                            }
                        }
                    );
                }
            }
        );

        if (frame_ptr == nullptr) {
            return false;
        }

        frame_ptr->Show(true);
        return true;
    }

private:
    void OnTokenReceived(wxThreadEvent& event) {
        auto* frame_ptr = dynamic_cast<ui::MainFrame*>(GetTopWindow());
        if (frame_ptr != nullptr) {
            frame_ptr->AppendToken(event.GetString().ToStdString(wxConvUTF8));
        }
    }

    std::unique_ptr<network::StreamWorker> m_worker_ptr;
};

} // namespace malama

wxIMPLEMENT_APP(malama::MalamaApp);

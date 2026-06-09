// /////////////////////////////////////////////////////////////////////////////
// Name:        src/main.cpp
// Purpose:     Main application entry point for malama native client
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include <wx/wx.h>
#include <new>
#include <string>
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
        spdlog::info("Initializing malama v0.0.6 Resizable Splitter Canvas Engine...");

        auto *frame_ptr = new (std::nothrow) ui::MainFrame(
            "malama Local UI Engine",
            wxDefaultPosition, 
            wxSize(constants::default_window_width, constants::default_window_height)
        );
        if (frame_ptr == nullptr) {
            return false;
        }
        
        frame_ptr->Show(true);

        auto client_ptr = std::make_unique<network::OllamaClient>(
            std::string(constants::default_ollama_endpoint)
        );
        m_worker_ptr = std::make_unique<network::StreamWorker>(std::move(client_ptr));

        // Launch a validation stream loop that simulates token generation chunks
        m_worker_ptr->InitializeGeneration(
            "qwen2.5-coder",
            std::vector<common::Message>{},
            [frame_ptr](std::string_view token) mutable {
                auto *event_ptr = new (std::nothrow) wxThreadEvent(ui::EVT_MALAMA_TOKEN); // NOLINT
                if (event_ptr != nullptr) {
                    // Inject a sample string chunk to simulate a live token stream
                    std::string payload(token);
                    if (payload.empty() || payload == " ") {
                        payload = " [token_chunk] ";
                    }
                    
                    event_ptr->SetString(wxString::FromUTF8(payload.data(), payload.size()));
                    
                    // Route ownership to allow the static handler to unpack component objects securely
                    event_ptr->SetEventObject(frame_ptr);
                    wxQueueEvent(frame_ptr, event_ptr);
                }
            }
        );

        return true;
    }

private:
    std::unique_ptr<network::StreamWorker> m_worker_ptr;
};

} // namespace malama

wxIMPLEMENT_APP(malama::MalamaApp);

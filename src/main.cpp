// /////////////////////////////////////////////////////////////////////////////
// Name:        src/main.cpp
// Purpose:     Main application entry point validating Glaze stream processing
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
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
        spdlog::info("Initializing malama v0.0.7 Glaze Streaming Parse Engine...");

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

        // Connect the interface callback channel to the stream worker
        m_worker_ptr->InitializeGeneration(
            "qwen2.5-coder",
            std::vector<common::Message>{},
            [frame_ptr](std::string_view parsed_token) mutable {
                auto *event_ptr = new (std::nothrow) wxThreadEvent(ui::EVT_MALAMA_TOKEN); // NOLINT
                if (event_ptr != nullptr) {
                    event_ptr->SetString(wxString::FromUTF8(parsed_token.data(), parsed_token.size()));
                    event_ptr->SetEventObject(frame_ptr);
                    wxQueueEvent(frame_ptr, event_ptr);
                }
            }
        );

        // Simulate incoming fragmented JSON chunk strings from a socket
        m_worker_ptr->IngestRawNetworkBytes("{\"response\":\"# include \",\"done\":false}\n{\"response\":\"<iostream>\\n\\n\",\"done\":false}\n");
        m_worker_ptr->IngestRawNetworkBytes("{\"response\":\"int \",\"done\":false}\n{\"response\":\"main() \",\"done\":false}\n{\"response\":\"{\\n\",\"done\":false}\n");
        m_worker_ptr->IngestRawNetworkBytes("{\"response\":\"    std::cout << \\\"Hello \",\"done\":false}\n{\"response\":\"Malama v0.0.7!\\\\n\\\";\\n\",\"done\":false}\n");
        m_worker_ptr->IngestRawNetworkBytes("{\"response\":\"    return 0;\\n\",\"done\":false}\n{\"response\":\"}\",\"done\":true}\n");

        return true;
    }

private:
    std::unique_ptr<network::StreamWorker> m_worker_ptr;
};

} // namespace malama

wxIMPLEMENT_APP(malama::MalamaApp);

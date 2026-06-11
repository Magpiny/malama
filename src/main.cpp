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
#include <wx/weakref.h>
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
        spdlog::info("Initializing malama v0.0.9 Glaze Streaming Parse Engine...");

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
            std::string(constants::default_ollama_host), 
            std::string(constants::default_ollama_port)
        );
        m_worker_ptr = std::make_unique<network::StreamWorker>(std::move(client_ptr));

      // Transmit a live generation request across the native TCP interface
        wxWeakRef<ui::MainFrame> weak_frame_ref(frame_ptr);
        m_worker_ptr->InitializeGeneration(
            constants::fallback_model_name,
            "Write a very short, two sentence haiku about C++ performance.",
            std::vector<common::Message>{},
            [weak_frame_ref](std::string_view parsed_token) mutable {
                if (!weak_frame_ref) {
                    return;
                }
                auto *event_ptr = new (std::nothrow) wxThreadEvent(ui::EVT_MALAMA_TOKEN); // NOLINT
                if (event_ptr != nullptr) {
                    event_ptr->SetString(wxString::FromUTF8(parsed_token.data(), parsed_token.size()));
                    event_ptr->SetEventObject(weak_frame_ref.get());
                    wxQueueEvent(weak_frame_ref.get(), event_ptr);
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

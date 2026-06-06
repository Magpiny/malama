// /////////////////////////////////////////////////////////////////////////////
// Name:        src/main.cpp
// Purpose:     Main application entry point for malama native client
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-06
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include <wx/wx.h>
#include <spdlog/spdlog.h>
#include "ui/main_frame.hpp"
#include "common/constants.hpp"

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
        spdlog::info("Initializing malama v0.0.2 Domain & Core Layouts Workspace...");

        // Instantiate the primary hardened UI window frame target
        auto *frame_ptr = new ui::MainFrame("malama Local UI Engine",
                                                     wxDefaultPosition, wxSize(constants::default_window_width,constants::default_window_height));
        frame_ptr->Show(true);

        return true;
    }
};

} // namespace malama

wxIMPLEMENT_APP(malama::MalamaApp);

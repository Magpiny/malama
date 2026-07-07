// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_chunks.hpp
// Purpose:     Glaze JSON schema definitions for Ollama API streaming frames
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3-or-later

#include <glaze/glaze.hpp>
#include <string>

namespace malama::network {

// Structure mapping the standard chunk response payload from /api/generate
struct OllamaGenerateChunk final {
    std::string response{};
    bool done{false};

    // Glaze compile-time reflection registration interface
    struct glaze {
        using T = OllamaGenerateChunk;
        static constexpr auto value = glz::object("response", &T::response, "done", &T::done);
    };
};

}  // namespace malama::network

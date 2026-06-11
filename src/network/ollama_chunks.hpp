// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_chunks.hpp
// Purpose:     Glaze JSON schema definitions for Ollama API streaming frames
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <glaze/glaze.hpp>

namespace malama::network {

// Structure mapping the standard chunk response payload from /api/generate
struct OllamaGenerateChunk final {
    std::string response{};
    bool done{false};

    // Glaze compile-time reflection registration interface
    struct glaze {
        using T = OllamaGenerateChunk;
        static constexpr auto value = glz::object(
            "response", &T::response,
            "done", &T::done
        );
    };
};

} // namespace malama::network

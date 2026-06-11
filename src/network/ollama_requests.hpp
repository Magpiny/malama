// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_requests.hpp
// Purpose:     Glaze JSON schema definitions for outbound Ollama API POSTs
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <glaze/glaze.hpp>

namespace malama::network {

struct OllamaGenerateRequest final {
    std::string model{};
    std::string prompt{};
    bool stream{true};

    struct glaze {
        using T = OllamaGenerateRequest;
        static constexpr auto value = glz::object(
            "model", &T::model,
            "prompt", &T::prompt,
            "stream", &T::stream
        );
    };
};

} // namespace malama::network

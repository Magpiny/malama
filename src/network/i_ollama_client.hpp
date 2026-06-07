// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/i_ollama_client.hpp
// Purpose:     Pure abstract interface for the Ollama network client
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include "common/types.hpp"
#include <expected>
#include <functional>
#include <stop_token>
#include <string_view>

namespace malama::network {

class IOllamaClient {
public:
    // Fixed: Explicitly declare default constructor to restore generation chains
    IOllamaClient() = default;
    virtual ~IOllamaClient() = default;

    // Rigidly prohibit copy mechanics at the interface to entirely stop slicing
    IOllamaClient(const IOllamaClient &) = delete;
    IOllamaClient &operator=(const IOllamaClient &) = delete;

    // Fixed: Enable defaulted move actions so derived implementations can be moved
    IOllamaClient(IOllamaClient &&) noexcept = default;
    IOllamaClient &operator=(IOllamaClient &&) noexcept = default;

    [[nodiscard]] virtual auto RequestStream(
        std::string_view model_name,
        const std::vector<common::Message> &historical_chain,
        std::move_only_function<void(std::string_view)> token_callback,
        std::stop_token cancellation_token
    ) noexcept -> std::expected<void, common::NetworkError> = 0;
};

} // namespace malama::network

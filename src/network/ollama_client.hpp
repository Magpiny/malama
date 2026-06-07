// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_client.hpp
// Purpose:     Asynchronous Ollama HTTP API interaction engine via Boost.Beast
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include "network/i_ollama_client.hpp"

namespace malama::network {

class OllamaClient final : public IOllamaClient {
public:
    explicit OllamaClient(std::string target_endpoint) noexcept;
    ~OllamaClient() override = default;

    OllamaClient(const OllamaClient &) = delete;
    OllamaClient &operator=(const OllamaClient &) = delete;

    OllamaClient(OllamaClient &&) noexcept = default;
    OllamaClient &operator=(OllamaClient &&) noexcept = default;

    [[nodiscard]] auto RequestStream(
        std::string_view model_name,
        const std::vector<common::Message> &historical_chain,
        std::move_only_function<void(std::string_view)> token_callback,
        std::stop_token cancellation_token
    ) noexcept -> std::expected<void, common::NetworkError> override;

private:
    std::string m_ollama_endpoint;
};

} // namespace malama::network

// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_requests.hpp
// Purpose:     Glaze JSON schema definitions for outbound Ollama API POSTs
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include <glaze/glaze.hpp>
#include <string>
#include <vector>

#include "common/constants.hpp"

namespace malama::network {

/**
 * @brief Represents an individual message turn inside an Ollama chat payload.
 */
struct ChatMessagePayload final {
    std::string m_role{};
    std::string m_content{};
    std::vector<std::string> m_images{};

    struct glaze {
        using Type = ChatMessagePayload;
        static constexpr auto value = glz::object("role", &Type::m_role, "content",
                                                  &Type::m_content, "images", &Type::m_images);
    };
};

/**
 * @brief Generation tuning options passed to Ollama model execution engine.
 */
struct OllamaOptionsPayload final {
    float m_temperature{constants::chat_temperature};
    float m_top_p{constants::top_p};
    int m_top_k{constants::top_k};
    float m_repeat_penalty{constants::rpt_penalty};
    std::uint32_t m_num_ctx{constants::max_chat_length};

    struct glaze {
        using Type = OllamaOptionsPayload;
        static constexpr auto value = glz::object(
            "temperature", &Type::m_temperature, "top_p", &Type::m_top_p, "top_k", &Type::m_top_k,
            "repeat_penalty", &Type::m_repeat_penalty, "num_ctx", &Type::m_num_ctx);
    };
};

/**
 * @brief Unified outbound HTTP request payload structure targeting Ollama /api/chat.
 */
struct OllamaChatRequest final {
    std::string m_model{};
    std::vector<ChatMessagePayload> m_messages{};
    bool m_stream{true};
    OllamaOptionsPayload m_options{};

    struct glaze {
        using Type = OllamaChatRequest;
        static constexpr auto value =
            glz::object("model", &Type::m_model, "messages", &Type::m_messages, "stream",
                        &Type::m_stream, "options", &Type::m_options);
    };
};

}  // namespace malama::network

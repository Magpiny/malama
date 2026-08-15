/////////////////////////////////////////////////////////////////////////////
// Name:        include/core/models.hpp
// Purpose:     Encapsulates application state domain entities and configurations
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>
#include <glaze/glaze.hpp>
#include <string>
#include <vector>

#include "common/constants.hpp"

namespace malama::core {

enum class MessageRole : std::uint8_t { System, User, Assistant };

struct ModelParameters {
    float m_temperature = constants::chat_temperature;
    float m_top_p = constants::top_p;
    int32_t m_top_k = constants::top_k;
    float m_repeat_penalty = constants::rpt_penalty;
    uint32_t m_num_ctx = constants::max_chat_length;

    std::string m_system_prompt;

    struct glaze {
        using Type = ModelParameters;
        static constexpr auto value =
            glz::object("temperature", &Type::m_temperature, "top_p", &Type::m_top_p, "top_k",
                        &Type::m_top_k, "repeat_penalty", &Type::m_repeat_penalty, "num_ctx",
                        &Type::m_num_ctx, "system_prompt", &Type::m_system_prompt);
    };
};

struct Message {
    std::string m_id{"0"};
    MessageRole m_role;
    std::string m_content;
    uint64_t m_timestamp{0};
    bool m_is_starred{false};  // Enables bookmarking specific code explanations

    struct glaze {
        using Type = Message;
        static constexpr auto value =
            glz::object("id", &Type::m_id, "role", &Type::m_role, "content", &Type::m_content,
                        "timestamp", &Type::m_timestamp, "is_starred", &Type::m_is_starred);
    };
};

struct SessionMetadata {
    std::string m_session_id;
    std::string m_title;
    uint64_t m_created_at{0};  // Needed for structural tracking sorting metrics
    uint64_t m_updated_at{0};
    bool m_is_pinned{false};         // Locks conversation to top of sidebar tracking list
    ModelParameters m_parameters{};  // Per-session hyperparameter configuration

    struct glaze {
        using Type = SessionMetadata;
        static constexpr auto value =
            glz::object("session_id", &Type::m_session_id, "title", &Type::m_title, "created_at",
                        &Type::m_created_at, "updated_at", &Type::m_updated_at, "is_pinned",
                        &Type::m_is_pinned, "parameters", &Type::m_parameters);
    };
};

struct ChatSession {
    SessionMetadata m_metadata;
    std::vector<Message> m_messages;

    struct glaze {
        using Type = ChatSession;
        static constexpr auto value =
            glz::object("metadata", &Type::m_metadata, "messages", &Type::m_messages);
    };
};

}  // namespace malama::core

// /////////////////////////////////////////////////////////////////////////////
// Name:        include/core/models.hpp
// Purpose:     Encapsulates application state domain entities and configurations
// Author:      Wanjare S. <samuewanjare@protonmail.com>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>
#include <glaze/glaze.hpp>
#include <string>
#include <vector>

namespace malama::core {

enum class MessageRole : std::uint8_t { System, User, Assistant };

struct Message {
    std::string m_id;
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
    bool m_is_pinned{false};  // Locks conversation to top of sidebar tracking list

    struct glaze {
        using Type = SessionMetadata;
        static constexpr auto value =
            glz::object("session_id", &Type::m_session_id, "title", &Type::m_title, "created_at",
                        &Type::m_created_at, "updated_at", &Type::m_updated_at, "is_pinned",
                        &Type::m_is_pinned);
    };
};

struct ChatSession {
    SessionMetadata m_metadata;
    std::vector<Message> m_messages;
};

}  // namespace malama::core

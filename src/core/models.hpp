#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <glaze/glaze.hpp>

namespace malama::core {

enum class MessageRole:std::uint8_t { System, User, Assistant };

struct Message {
    std::string m_id;
    MessageRole m_role;
    std::string m_content;
    uint64_t m_timestamp;
    bool m_is_starred{false}; // NEW: For starring specific responses

    struct glaze {
        using T = Message;
        static constexpr auto value = glz::object(
            "id", &T::m_id,
            "role", &T::m_role,
            "content", &T::m_content,
            "timestamp", &T::m_timestamp,
            "is_starred", &T::m_is_starred
        );
    };
};

struct SessionMetadata {
    std::string m_session_id;
    std::string m_title;
    uint64_t m_updated_at;
    bool m_is_pinned{false}; // NEW: For pinning to top of sidebar

    struct glaze {
        using T = SessionMetadata;
        static constexpr auto value = glz::object(
            "session_id", &T::m_session_id,
            "title", &T::m_title,
            "updated_at", &T::m_updated_at,
            "is_pinned", &T::m_is_pinned
        );
    };
};

struct ChatSession {
    SessionMetadata m_metadata;
    std::vector<Message> m_messages;
};

} // namespace malama::core

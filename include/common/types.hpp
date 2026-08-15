// /////////////////////////////////////////////////////////////////////////////
// Name:        include/common/types.hpp
// Purpose:     Core type definitions and scoped error matrices for malama
// Author:      Wanjare <samuelwanjare@protonmail.com>
// Created:     2026-06-06
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>
#include <string>
#include <vector>

#include "constants.hpp"

namespace malama::common {

// Optimized: Explicitly pinned to 1 byte to minimize structure allocation overheads
enum class AuthorRole : std::uint8_t { System, User, Assistant };

// Optimized: Explicitly pinned to 1 byte for non-throwing return paths
enum class ConfigError : std::uint8_t {
    FileNotFound,
    PermissionDenied,
    InvalidJsonSyntax,
    SchemaMismatch
};

// New: Explicitly pinned to 1 byte network failure classification engine
enum class NetworkError : std::uint8_t {
    HostUnreachable,
    InvalidResponse,
    StreamInterrupted,
    ExecutionCancelled
};

struct Message {
    AuthorRole m_role;
    std::string m_content;
};

struct ModelInfo {
    std::string m_name;
    std::string m_details;
    std::size_t m_parameter_size{0};
};

struct Session {
    std::string m_uuid;
    std::string m_title;
    std::vector<Message> m_historical_chain;
};

struct AppSettings {
    std::string m_ollama_endpoint{constants::default_ollama_endpoint};
    std::string m_fallback_model{constants::fallback_model_name};
    std::size_t m_context_window_limit{constants::default_context_window_limit};
    bool m_stream_tokens{true};
};

struct ModelParameters {
    float m_temperature = constants::chat_temperature;
    float m_top_p = constants::top_p;
    int32_t m_top_k = constants::top_k;
    float m_repeat_penalty = constants::rpt_penalty;
    uint32_t m_num_ctx = constants::max_chat_length;
    std::string m_system_prompt;
};

struct SessionMetadata {
    std::string m_session_id;
    std::string m_title;
    uint64_t m_created_at{0};
    uint64_t m_updated_at{0};
    bool m_is_pinned{false};
    ModelParameters m_parameters{};
};

struct ChatSession {
    SessionMetadata m_metadata{};
    std::vector<Message> m_messages;
};

/**
 * @brief Configuration parameters for a single LLM session thread.
 */
struct SessionParameters {
    float m_temperature{constants::chat_temperature};
    float m_top_p{constants::top_p};
    int32_t m_top_k{constants::top_k};
    float m_repeat_penalty{constants::rpt_penalty};
    uint32_t m_num_ctx{constants::max_chat_length};
    std::string m_system_prompt{};
};

}  // namespace malama::common

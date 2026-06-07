// /////////////////////////////////////////////////////////////////////////////
// Name:        src/common/types.hpp
// Purpose:     Core type definitions and scoped error matrices for malama
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-06
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include "common/constants.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace malama::common {

// Optimized: Explicitly pinned to 1 byte to minimize structure allocation overheads
enum class AuthorRole : std::uint8_t {
    System,
    User,
    Assistant
};

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

} // namespace malama::common

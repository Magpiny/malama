// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/parsers/parser_types.hpp
// Purpose:     Common enumeration primitives and error types for document parsing
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>
#include <string>

namespace malama::engine::storage::parsers {

enum class ParserErrorCode : std::uint8_t {
    SUCCESS,
    FILE_READ_ERROR,
    ARCHIVE_EXTRACT_ERROR,
    MALFORMED_CONTENT,
    UNSUPPORTED_FORMAT
};

struct ParserResult final {
    ParserErrorCode m_error_code{ParserErrorCode::SUCCESS};
    std::string m_extracted_text;
    std::string m_log_message;
};

}  // namespace malama::engine::storage::parsers

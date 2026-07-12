// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/plain_text_parser.cpp
// Purpose:     Implements narrow standard stream readers with conversion guards
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/plain_text_parser.hpp"

#include <filesystem>
#include <fstream>

namespace malama::engine::storage::parsers {

auto PlainTextParser::parse_file(const std::string &file_path) noexcept -> ParserResult {
    std::error_code error_state;
    if (!std::filesystem::exists(file_path, error_state)) {
        return ParserResult{.m_error_code = ParserErrorCode::FILE_READ_ERROR,
                            .m_extracted_text = "",
                            .m_log_message = "Target plain text asset file path is non-existent."};
    }

    std::ifstream stream_handle(file_path, std::ios::in | std::ios::binary);
    if (!stream_handle.is_open()) {
        return ParserResult{.m_error_code = ParserErrorCode::FILE_READ_ERROR,
                            .m_extracted_text = "",
                            .m_log_message = "Failed to secure an open system stream handle."};
    }

    std::string text_accumulator;
    stream_handle.seekg(0, std::ios::end);
    const std::streamsize stream_size = stream_handle.tellg();
    if (stream_size > 0) {
        text_accumulator.resize(static_cast<std::size_t>(stream_size));
        stream_handle.seekg(0, std::ios::beg);
        stream_handle.read(text_accumulator.data(), stream_size);
    }

    return ParserResult{.m_error_code = ParserErrorCode::SUCCESS,
                        .m_extracted_text = std::move(text_accumulator),
                        .m_log_message = "Flat documentation payload ingestion completed."};
}

}  // namespace malama::engine::storage::parsers

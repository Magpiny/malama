// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/xlsx_parser.cpp
// Purpose:     Pugixml matching for sharedString indexes inside row structures
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/xlsx_parser.hpp"

#include <pugixml.hpp>

#include "engine/storage/parsers/archive_reader.hpp"

namespace malama::engine::storage::parsers {

auto XlsxParser::parse_file(const std::string &file_path) noexcept -> ParserResult {
    ArchiveReader zip_extractor;
    auto shared_strings_buffer =
        zip_extractor.extract_file_to_buffer(file_path, "xl/sharedStrings.xml");

    if (!shared_strings_buffer.has_value()) {
        return ParserResult{.m_error_code = ParserErrorCode::ARCHIVE_EXTRACT_ERROR,
                            .m_extracted_text = "",
                            .m_log_message = "Shared matrix string map table data unreadable."};
    }

    pugi::xml_document string_table;
    string_table.load_buffer(shared_strings_buffer->data(), shared_strings_buffer->size());

    std::string text_accumulator;
    // Microsoft Excel layouts map grid tokens sequentially inside <sst><si><t> blocks
    const pugi::xpath_node_set tabular_cells = string_table.select_nodes("//sst/si/t/text()");
    for (const pugi::xpath_node &cell : tabular_cells) {
        text_accumulator.append(cell.node().value());
        text_accumulator.append(" | ");
    }

    return ParserResult{.m_error_code = ParserErrorCode::SUCCESS,
                        .m_extracted_text = std::move(text_accumulator),
                        .m_log_message = "Tabular XLSX data extracted successfully."};
}

}  // namespace malama::engine::storage::parsers

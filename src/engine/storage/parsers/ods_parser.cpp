// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/ods_parser.cpp
// Purpose:     Extracts spreadsheet content from table:table-cell elements
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/ods_parser.hpp"

#include <pugixml.hpp>

#include "engine/storage/parsers/archive_reader.hpp"

namespace malama::engine::storage::parsers {

auto OdsParser::parse_file(const std::string &file_path) noexcept -> ParserResult {
    ArchiveReader zip_extractor;
    auto content_buffer = zip_extractor.extract_file_to_buffer(file_path, "content.xml");

    if (!content_buffer.has_value()) {
        return ParserResult{.m_error_code = ParserErrorCode::ARCHIVE_EXTRACT_ERROR,
                            .m_extracted_text = "",
                            .m_log_message = "Tabular content.xml tracking schema lost."};
    }

    pugi::xml_document ods_blueprint;
    ods_blueprint.load_buffer(content_buffer->data(), content_buffer->size());

    std::string text_accumulator;
    // ODS structures table contents within sequential <table:table-cell> text strings
    const pugi::xpath_node_set matrix_cells =
        ods_blueprint.select_nodes("//table:table-row/table:table-cell//text()");
    for (const pugi::xpath_node &cell : matrix_cells) {
        text_accumulator.append(cell.node().value());
        text_accumulator.append(" \t ");
    }

    return ParserResult{.m_error_code = ParserErrorCode::SUCCESS,
                        .m_extracted_text = std::move(text_accumulator),
                        .m_log_message = "Tabular cell map parsed cleanly."};
}

}  // namespace malama::engine::storage::parsers

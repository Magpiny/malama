// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/odt_parser.cpp
// Purpose:     Pugixml query mapping for extracting text from content.xml files
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/odt_parser.hpp"

#include <pugixml.hpp>

#include "engine/storage/parsers/archive_reader.hpp"

namespace malama::engine::storage::parsers {

auto OdtParser::parse_file(const std::string &file_path) noexcept -> ParserResult {
    ArchiveReader zip_extractor;
    auto asset_buffer = zip_extractor.extract_file_to_buffer(file_path, "content.xml");

    if (!asset_buffer.has_value()) {
        return ParserResult{
            .m_error_code = ParserErrorCode::ARCHIVE_EXTRACT_ERROR,
            .m_extracted_text = "",
            .m_log_message = "OASIS open asset manifest layout missing content.xml"};
    }

    pugi::xml_document odt_blueprint;
    if (!odt_blueprint.load_buffer(asset_buffer->data(), asset_buffer->size())) {
        return ParserResult{
            .m_error_code = ParserErrorCode::MALFORMED_CONTENT,
            .m_extracted_text = "",
            .m_log_message = "Unbalanced open layout document configuration parsed."};
    }

    std::string text_accumulator;
    // ODF standard structures layout information within <text:p> nodes
    const pugi::xpath_node_set text_nodes = odt_blueprint.select_nodes("//text:p/text()");
    for (const pugi::xpath_node &node : text_nodes) {
        text_accumulator.append(node.node().value());
        text_accumulator.append("\n");
    }

    return ParserResult{.m_error_code = ParserErrorCode::SUCCESS,
                        .m_extracted_text = std::move(text_accumulator),
                        .m_log_message = "Oasis ODT content tracking array sync completed."};
}

}  // namespace malama::engine::storage::parsers

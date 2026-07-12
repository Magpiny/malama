// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/xml_parser.cpp
// Purpose:     Pugixml walking loops gathering contiguous visual text nodes
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/xml_parser.hpp"

#include <fstream>
#include <pugixml.hpp>
#include <vector>

namespace malama::engine::storage::parsers {

auto XmlParser::parse_file(const std::string &file_path) noexcept -> ParserResult {
    std::ifstream stream_handle(file_path, std::ios::in | std::ios::binary);
    if (!stream_handle.is_open()) {
        return ParserResult{.m_error_code = ParserErrorCode::FILE_READ_ERROR,
                            .m_extracted_text = "",
                            .m_log_message = "Failed to read standard structural XML file."};
    }

    std::vector<char> context_buffer((std::istreambuf_iterator<char>(stream_handle)),
                                     std::istreambuf_iterator<char>());
    return parse_xml_buffer(context_buffer.data(), context_buffer.size());
}

auto XmlParser::parse_xml_buffer(const char *buffer, std::size_t size) noexcept -> ParserResult {
    pugi::xml_document xml_blueprint;
    const pugi::xml_parse_result result = xml_blueprint.load_buffer(buffer, size);
    if (!result) {
        return ParserResult{.m_error_code = ParserErrorCode::MALFORMED_CONTENT,
                            .m_extracted_text = "",
                            .m_log_message = "Pugixml reported an unbalanced token error context."};
    }

    std::string text_accumulator;
    // FIXED: Correctly capture the pugixml XPath collection node wrapper instances
    const pugi::xpath_node_set text_nodes = xml_blueprint.select_nodes("//text()");
    for (const pugi::xpath_node &xpath_node : text_nodes) {
        text_accumulator.append(xpath_node.node().value());
        text_accumulator.append(" ");
    }

    return ParserResult{.m_error_code = ParserErrorCode::SUCCESS,
                        .m_extracted_text = std::move(text_accumulator),
                        .m_log_message = "Generic XML node tree mapping completed."};
}

}  // namespace malama::engine::storage::parsers

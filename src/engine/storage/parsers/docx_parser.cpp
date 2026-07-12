// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/docx_parser.cpp
// Purpose:     Decompresses internal word/document.xml layers for scanning
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/docx_parser.hpp"

#include <pugixml.hpp>

#include "engine/storage/parsers/archive_reader.hpp"

namespace malama::engine::storage::parsers {

auto DocxParser::parse_file(const std::string &file_path) noexcept -> ParserResult {
    ArchiveReader zip_extractor;
    auto document_buffer = zip_extractor.extract_file_to_buffer(file_path, "word/document.xml");

    if (!document_buffer.has_value()) {
        return ParserResult{.m_error_code = ParserErrorCode::ARCHIVE_EXTRACT_ERROR,
                            .m_extracted_text = "",
                            .m_log_message = "Required word/document.xml token layer missing."};
    }

    pugi::xml_document docx_blueprint;
    const auto parse_status =
        docx_blueprint.load_buffer(document_buffer->data(), document_buffer->size());
    if (!parse_status) {
        return ParserResult{.m_error_code = ParserErrorCode::MALFORMED_CONTENT,
                            .m_extracted_text = "",
                            .m_log_message = "Malformed Open XML specification content detected."};
    }

    std::string text_accumulator;
    // Microsoft Word structures paragraph segments within matching <w:t> tags
    const pugi::xpath_node_set text_nodes = docx_blueprint.select_nodes("//w:t/text()");
    for (const pugi::xpath_node &node : text_nodes) {
        text_accumulator.append(node.node().value());
        text_accumulator.append(" ");
    }

    return ParserResult{.m_error_code = ParserErrorCode::SUCCESS,
                        .m_extracted_text = std::move(text_accumulator),
                        .m_log_message = "DOCX paragraph stream generated successfully."};
}

}  // namespace malama::engine::storage::parsers

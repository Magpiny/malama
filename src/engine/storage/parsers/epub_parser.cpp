// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/epub_parser.cpp
// Purpose:     Parses container manifests to gather structural context segments
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/epub_parser.hpp"

#include <pugixml.hpp>

#include "engine/storage/parsers/archive_reader.hpp"

namespace malama::engine::storage::parsers {

auto EpubParser::parse_file(const std::string &file_path) noexcept -> ParserResult {
    ArchiveReader zip_extractor;
    auto meta_buffer = zip_extractor.extract_file_to_buffer(file_path, "META-INF/container.xml");

    if (!meta_buffer.has_value()) {
        return ParserResult{.m_error_code = ParserErrorCode::ARCHIVE_EXTRACT_ERROR,
                            .m_extracted_text = "",
                            .m_log_message =
                                "Required internal epub layout directory structure "
                                "container invalid."};
    }

    pugi::xml_document container_blueprint;
    if (!container_blueprint.load_buffer(meta_buffer->data(), meta_buffer->size())) {
        return ParserResult{.m_error_code = ParserErrorCode::MALFORMED_CONTENT,
                            .m_extracted_text = "",
                            .m_log_message = "Epub structural layout context validation break."};
    }

    const pugi::xml_node root_node =
        container_blueprint.select_node("//container/rootfiles/rootfile").node();
    const std::string manifest_path = root_node.attribute("full-path").as_string();

    if (manifest_path.empty()) {
        return ParserResult{.m_error_code = ParserErrorCode::MALFORMED_CONTENT,
                            .m_extracted_text = "",
                            .m_log_message = "Manifest layout tracking point reference failed."};
    }

    auto opf_buffer = zip_extractor.extract_file_to_buffer(file_path, manifest_path);
    if (!opf_buffer.has_value()) {
        return ParserResult{.m_error_code = ParserErrorCode::ARCHIVE_EXTRACT_ERROR,
                            .m_extracted_text = "",
                            .m_log_message = "Target inner OPF metadata book index missing."};
    }

    pugi::xml_document opf_blueprint;
    opf_blueprint.load_buffer(opf_buffer->data(), opf_buffer->size());

    std::string text_accumulator;
    const pugi::xpath_node_set spine_items = opf_blueprint.select_nodes("//package/manifest/item");

    for (const pugi::xpath_node &item_node : spine_items) {
        const std::string media_type = item_node.node().attribute("media-type").as_string();
        if (media_type == "application/xhtml+xml") {
            const std::string structural_href = item_node.node().attribute("href").as_string();

            auto content_buffer =
                zip_extractor.extract_file_to_buffer(file_path, "OEBPS/" + structural_href);
            if (content_buffer.has_value()) {
                pugi::xml_document text_blueprint;
                text_blueprint.load_buffer(content_buffer->data(), content_buffer->size());

                // FIXED: Reconciled loop variables to expect valid XPath collections
                const pugi::xpath_node_set text_nodes = text_blueprint.select_nodes("//text()");
                for (const pugi::xpath_node &xpath_node : text_nodes) {
                    text_accumulator.append(xpath_node.node().value());
                    text_accumulator.append(" ");
                }
            }
        }
    }

    return ParserResult{.m_error_code = ParserErrorCode::SUCCESS,
                        .m_extracted_text = std::move(text_accumulator),
                        .m_log_message = "Digital eBook content chapters unpacked."};
}

}  // namespace malama::engine::storage::parsers

// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/pdf_parser.cpp
// Purpose:     Poppler document object loop iterating over internal pages
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/pdf_parser.hpp"

#include <memory>
#include <poppler-document.h>
#include <poppler-page.h>

namespace malama::engine::storage::parsers {

auto PdfParser::parse_file(const std::string &file_path) noexcept -> ParserResult {
    std::unique_ptr<poppler::document> document_blueprint(
        poppler::document::load_from_file(file_path));

    if (document_blueprint == nullptr) {
        return ParserResult{.m_error_code = ParserErrorCode::FILE_READ_ERROR,
                            .m_extracted_text = "",
                            .m_log_message = "Poppler framework rejected the PDF file structure."};
    }

    if (document_blueprint->is_locked()) {
        return ParserResult{.m_error_code = ParserErrorCode::FILE_READ_ERROR,
                            .m_extracted_text = "",
                            .m_log_message = "Targetdocument requires explicit decryption pass."};
    }

    std::string text_accumulator;
    const int target_pages_count = document_blueprint->pages();

    for (int index = 0; index < target_pages_count; ++index) {
        std::unique_ptr<poppler::page> current_page_ptr(document_blueprint->create_page(index));
        if (current_page_ptr != nullptr) {
            const poppler::ustring page_string = current_page_ptr->text();

            const poppler::byte_array utf8_bytes = page_string.to_utf8();
            const std::string standard_string(utf8_bytes.begin(), utf8_bytes.end());

            text_accumulator.append(standard_string);
            text_accumulator.append("\n");
        }
    }

    if (text_accumulator.empty()) {
        return ParserResult{
            .m_error_code = ParserErrorCode::MALFORMED_CONTENT,
            .m_extracted_text = "",
            .m_log_message = "PDF contains no extractable text layer (scanned/image-only)."};
    }

    return ParserResult{.m_error_code = ParserErrorCode::SUCCESS,
                        .m_extracted_text = std::move(text_accumulator),
                        .m_log_message = "Universal poppler string tracking sequence completed."};
}

}  // namespace malama::engine::storage::parsers

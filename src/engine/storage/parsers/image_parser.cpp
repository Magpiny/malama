// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/image_parser.cpp
// Purpose:     Boost.GIL dimensions extractor preventing runtime VRAM overflows
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/image_parser.hpp"

#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/image.hpp>
#include <filesystem>
#include <format>

namespace malama::engine::storage::parsers {

auto ImageParser::parse_file(const std::string &file_path) noexcept -> ParserResult {
    const std::filesystem::path generic_path(file_path);
    const std::string file_extension = generic_path.extension().string();

    std::size_t dimension_width = 0UZ;
    std::size_t dimension_height = 0UZ;

    try {
        if (file_extension == ".png") {
            // FIXED: Provided explicit settings instances to satisfy modern Boost I/O layouts
            const auto backend = boost::gil::read_image_info(
                file_path, boost::gil::image_read_settings<boost::gil::png_tag>{});
            dimension_width = static_cast<std::size_t>(backend._info._width);
            dimension_height = static_cast<std::size_t>(backend._info._height);
        } else if (file_extension == ".jpg" || file_extension == ".jpeg") {
            const auto backend = boost::gil::read_image_info(
                file_path, boost::gil::image_read_settings<boost::gil::jpeg_tag>{});
            dimension_width = static_cast<std::size_t>(backend._info._width);
            dimension_height = static_cast<std::size_t>(backend._info._height);
        } else {
            return ParserResult{.m_error_code = ParserErrorCode::UNSUPPORTED_FORMAT,
                                .m_extracted_text = "",
                                .m_log_message = "Vision codec format validation missing."};
        }
    } catch (...) {
        return ParserResult{.m_error_code = ParserErrorCode::MALFORMED_CONTENT,
                            .m_extracted_text = "",
                            .m_log_message = "Boost GIL reported binary layout header error."};
    }

    std::string metadata_summary = std::format(
        "[Multimodal Vision Target Metadata Signature | Format: {} | Width: {}px | Height: {}px]",
        file_extension, dimension_width, dimension_height);

    return ParserResult{.m_error_code = ParserErrorCode::SUCCESS,
                        .m_extracted_text = std::move(metadata_summary),
                        .m_log_message = "Vision coordinate map dimensions analyzed safely."};
}

}  // namespace malama::engine::storage::parsers

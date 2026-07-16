// /////////////////////////////////////////////////////////////////////////////
// Name:        tests/unit/test_mime_detector.cpp
// Purpose:     Validates image parser file routing and error boundaries
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-16
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "engine/storage/parsers/image_parser.hpp"

SCENARIO("Image parser handles file parsing boundaries safely", "[mime]") {
    GIVEN("An active image parser environment") {
        malama::engine::storage::parsers::ImageParser image_parser_instance;
        WHEN("A non-existent file with an unsupported extension is parsed") {
            const std::string invalid_file_path{"non_existent_file.txt"};
            const auto parse_result = image_parser_instance.parse_file(invalid_file_path);

            THEN("The parser must reject it with an unsupported format error") {
                REQUIRE(parse_result.m_error_code ==
                        malama::engine::storage::parsers::ParserErrorCode::UNSUPPORTED_FORMAT);
                REQUIRE(parse_result.m_extracted_text.empty() == true);
            }
        }

        WHEN("A non-existent file with a supported PNG extension is parsed") {
            const std::string missing_png_path{"non_existent_file.png"};
            const auto parse_result = image_parser_instance.parse_file(missing_png_path);

            THEN("The parser catches the Boost GIL throw and flags malformed content") {
                REQUIRE(parse_result.m_error_code ==
                        malama::engine::storage::parsers::ParserErrorCode::MALFORMED_CONTENT);
                REQUIRE(parse_result.m_extracted_text.empty() == true);
                REQUIRE(parse_result.m_log_message ==
                        "Boost GIL reported binary layout header error.");
            }
        }

        WHEN("A non-existent file with a supported JPEG extension is parsed") {
            const std::string missing_jpeg_path{"non_existent_file.jpg"};
            const auto parse_result = image_parser_instance.parse_file(missing_jpeg_path);

            THEN("The parser catches the Boost GIL throw and flags malformed content") {
                REQUIRE(parse_result.m_error_code ==
                        malama::engine::storage::parsers::ParserErrorCode::MALFORMED_CONTENT);
                REQUIRE(parse_result.m_extracted_text.empty() == true);
                REQUIRE(parse_result.m_log_message ==
                        "Boost GIL reported binary layout header error.");
            }
        }
    }
}

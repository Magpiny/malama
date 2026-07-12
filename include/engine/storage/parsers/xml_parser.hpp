// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/parsers/xml_parser.hpp
// Purpose:     Structured generic XML layout parsing controller via pugixml
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/i_text_parser.hpp"

namespace malama::engine::storage::parsers {

class XmlParser : public ITextParser {
   public:
    XmlParser() = default;
    XmlParser(XmlParser &&) = delete;
    XmlParser &operator=(const XmlParser &) = delete;
    XmlParser &operator=(XmlParser &&) = delete;
    XmlParser(const XmlParser &) = delete;
    ~XmlParser() override = default;

    [[nodiscard]] auto parse_file(const std::string &file_path) noexcept -> ParserResult override;

   protected:
    [[nodiscard]] auto parse_xml_buffer(const char *buffer, std::size_t size) noexcept
        -> ParserResult;
};

}  // namespace malama::engine::storage::parsers

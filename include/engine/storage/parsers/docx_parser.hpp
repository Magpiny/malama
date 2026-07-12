// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/parsers/docx_parser.hpp
// Purpose:     Extracts paragraphs from Microsoft Open XML document containers
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/i_text_parser.hpp"

namespace malama::engine::storage::parsers {

class DocxParser final : public ITextParser {
   public:
    DocxParser() = default;
    DocxParser(const DocxParser &) = delete;
    DocxParser(DocxParser &&) = delete;
    DocxParser &operator=(const DocxParser &) = delete;
    DocxParser &operator=(DocxParser &&) = delete;
    ~DocxParser() override = default;

    [[nodiscard]] auto parse_file(const std::string &file_path) noexcept -> ParserResult override;
};

}  // namespace malama::engine::storage::parsers

// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/parsers/pdf_parser.hpp
// Purpose:     Interrogates Adobe PDF text strings utilizing external poppler-cpp
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/i_text_parser.hpp"

namespace malama::engine::storage::parsers {

class PdfParser final : public ITextParser {
   public:
    PdfParser() = default;
    PdfParser(const PdfParser &) = delete;
    PdfParser(PdfParser &&) = delete;
    PdfParser &operator=(const PdfParser &) = delete;
    PdfParser &operator=(PdfParser &&) = delete;
    ~PdfParser() override = default;

    [[nodiscard]] auto parse_file(const std::string &file_path) noexcept -> ParserResult override;
};

}  // namespace malama::engine::storage::parsers

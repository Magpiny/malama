// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/parsers/plain_text_parser.hpp
// Purpose:     Loads and sanitizes flat raw plain text documents
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/i_text_parser.hpp"

namespace malama::engine::storage::parsers {

class PlainTextParser final : public ITextParser {
   public:
    PlainTextParser() = default;
    PlainTextParser(const PlainTextParser &) = delete;
    PlainTextParser(PlainTextParser &&) = delete;
    PlainTextParser &operator=(const PlainTextParser &) = delete;
    PlainTextParser &operator=(PlainTextParser &&) = delete;
    ~PlainTextParser() override = default;

    [[nodiscard]] auto parse_file(const std::string &file_path) noexcept -> ParserResult override;
};

}  // namespace malama::engine::storage::parsers

// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/parsers/i_text_parser.hpp
// Purpose:     Pure abstract interface declaration for text extractors
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include "engine/storage/parsers/parser_types.hpp"

namespace malama::engine::storage::parsers {

class ITextParser {
   public:
    ITextParser() = default;
    virtual ~ITextParser() = default;

    ITextParser(const ITextParser &) = delete;
    auto operator=(const ITextParser &) -> ITextParser & = delete;
    ITextParser(ITextParser &&) noexcept = default;
    auto operator=(ITextParser &&) noexcept -> ITextParser & = default;

    [[nodiscard]] virtual auto parse_file(const std::string &file_path) noexcept
        -> ParserResult = 0;
};

}  // namespace malama::engine::storage::parsers

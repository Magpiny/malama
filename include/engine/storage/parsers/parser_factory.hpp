// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/parsers/parser_factory.hpp
// Purpose:     Decoupled architectural map factory matching formats to objects
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>
#include <string>

#include "engine/storage/parsers/i_text_parser.hpp"

namespace malama::engine::storage::parsers {

class ParserFactory final {
   public:
    ParserFactory() = default;
    ParserFactory(const ParserFactory &) = default;
    ParserFactory(ParserFactory &&) = delete;
    ParserFactory &operator=(const ParserFactory &) = default;
    ParserFactory &operator=(ParserFactory &&) = delete;
    ~ParserFactory() = default;

    [[nodiscard]] static auto resolve_parser(const std::string &file_path) noexcept
        -> std::unique_ptr<ITextParser>;
};

}  // namespace malama::engine::storage::parsers

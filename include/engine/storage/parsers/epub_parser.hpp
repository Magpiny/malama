// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/parsers/epub_parser.hpp
// Purpose:     Universal electronic publication (eBook) format parser unit
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include "engine/storage/parsers/i_text_parser.hpp"

namespace malama::engine::storage::parsers {

class EpubParser final : public ITextParser {
   public:
    EpubParser() = default;
    ~EpubParser() override = default;

    [[nodiscard]] auto parse_file(const std::string &file_path) noexcept -> ParserResult override;
};

}  // namespace malama::engine::storage::parsers

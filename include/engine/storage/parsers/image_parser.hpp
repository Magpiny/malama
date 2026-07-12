// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/parsers/image_parser.hpp
// Purpose:     Validates vision metadata boundaries using Boost.GIL
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/i_text_parser.hpp"

namespace malama::engine::storage::parsers {

class ImageParser final : public ITextParser {
   public:
    ImageParser() = default;
    ImageParser(const ImageParser &) = delete;
    ImageParser(ImageParser &&) = delete;
    ImageParser &operator=(const ImageParser &) = delete;
    ImageParser &operator=(ImageParser &&) = delete;
    ~ImageParser() override = default;

    [[nodiscard]] auto parse_file(const std::string &file_path) noexcept -> ParserResult override;
};

}  // namespace malama::engine::storage::parsers

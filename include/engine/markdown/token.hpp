// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/token.hpp
// Purpose:     Strongly typed token architecture for the markdown parsing engine
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-04
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>
#include <string>

namespace malama::engine::markdown {

enum class token_type : std::uint8_t {
    paragraph,
    header_1,
    header_2,
    header_3,
    header_4,
    divider,
    list_unordered,
    list_ordered,
    code_block
};

struct Token final {
    token_type m_type{token_type::paragraph};
    std::string m_content;
    std::string m_language;
};

}  // namespace malama::engine::markdown

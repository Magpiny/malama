// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/token.hpp
// Purpose:     Strongly typed token architecture for the markdown parsing engine
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-04
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

// SPDX-License-Identifier: Apache-2.0

namespace malama::engine::markdown {

enum class token_type {
    paragraph,
    header_1,
    header_2,
    header_3,
    divider,
    list_unordered,
    list_ordered,
    code_block
};

struct Token final {
    token_type m_type{token_type::paragraph};
    std::string m_content{};
    std::string m_language{};
};

} // namespace malama::engine::markdown

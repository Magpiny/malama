// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/markdown/pipeline.hpp
// Purpose:     Markdown syntax highlighting pipeline
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "config/config_manager.hpp"
#include "engine/markdown/syntax_registry.hpp"

namespace malama::engine::markdown {

enum class token_type : std::uint8_t {
    paragraph,
    header_1,
    header_2,
    header_3,
    list_unordered,
    list_ordered,
    code_block,
    divider
};

struct Token final {
    token_type m_type;
    std::string m_content;
    std::string m_language; // Only used for code_block logs
};

class Pipeline final {
public:
    explicit Pipeline(config::AppearanceConfig theme) noexcept;
    ~Pipeline() = default;

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) noexcept = delete;
    Pipeline& operator=(Pipeline&&) noexcept = delete;

    [[nodiscard]] auto process(std::string_view raw_markdown) const -> std::string;

private:
    [[nodiscard]] static auto tokenize(std::string_view text) -> std::vector<Token>;
    [[nodiscard]] auto decorate_code_block(std::string_view code,
                                           const std::string& lang) const -> std::string;
    [[nodiscard]] auto decorate_inline_text(std::string_view text) const -> std::string;
    [[nodiscard]] auto emit(const std::vector<Token>& tokens) const -> std::string;

    config::AppearanceConfig m_theme;
    SyntaxRegistry m_registry;
};

} // namespace malama::engine::markdown

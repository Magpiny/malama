// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/pipeline.hpp
// Purpose:     Markdown syntax highlighting pipeline
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-01
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <functional>
#include "config/config_manager.hpp"
#include "engine/markdown/syntax_registry.hpp"
#include "engine/markdown/token.hpp"

// SPDX-License-Identifier: Apache-2.0

namespace malama::engine::markdown {

class Pipeline final {
public:
    explicit Pipeline(config::AppearanceConfig theme) noexcept;
    ~Pipeline() = default;

    Pipeline(const Pipeline&) = default;
    auto operator=(const Pipeline&) -> Pipeline& = default;
    Pipeline(Pipeline&&) noexcept = default;
    auto operator=(Pipeline&&) noexcept -> Pipeline& = default;

    [[nodiscard]] auto process(std::string_view raw_markdown) const -> std::string;

private:
    [[nodiscard]] auto tokenize(std::string_view text) const -> std::vector<Token>;
    [[nodiscard]] auto decorate_inline_text(std::string_view text) const -> std::string;
    [[nodiscard]] auto decorate_code_block(std::string_view code, const std::string& lang) const -> std::string;
    [[nodiscard]] auto emit(const std::vector<Token>& tokens) const -> std::string;

    // Decoupled Structural Handler Routines minimizing Cognitive Complexity
    void handle_header(const Token& token_ref, std::string& html_output, std::string_view size_modifier) const;
    void handle_list(const Token& token_ref, std::string& html_output, bool& active_list_flag, bool& alternative_list_flag, std::string_view list_tag) const;
    void handle_paragraph(const Token& token_ref, std::string& html_output) const;
    void handle_code_block(const Token& token_ref, std::string& html_output) const;
    void handle_divider(const Token& token_ref, std::string& html_output) const;
    
    [[nodiscard]] auto scan_and_emit_table(const std::vector<Token>& tokens, size_t& current_idx, std::string& html_output) const -> bool;

    config::AppearanceConfig m_theme;
    SyntaxRegistry m_registry;
};

} // namespace malama::engine::markdown

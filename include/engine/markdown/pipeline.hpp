// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/pipeline.hpp
// Purpose:     Decoupled zero-allocation markdown pipeline header
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-01
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "config/config_manager.hpp"
#include "engine/markdown/syntax_registry.hpp"
#include "engine/markdown/token.hpp"

namespace malama::engine::markdown {

struct ListStatePair final {
    std::reference_wrapper<bool> m_is_active;
    std::reference_wrapper<bool> m_is_alternative;
};

class Pipeline final {
   public:
    explicit Pipeline(config::AppearanceConfig theme) noexcept;
    ~Pipeline() = default;

    Pipeline(const Pipeline &) = default;
    auto operator=(const Pipeline &) -> Pipeline & = default;
    Pipeline(Pipeline &&) noexcept = default;
    auto operator=(Pipeline &&) noexcept -> Pipeline & = default;

    [[nodiscard]] auto process(std::string_view raw_markdown) const -> std::string;

   private:
    // Fixes Issue 7: Utility token factory made static
    [[nodiscard]] static auto tokenize(std::string_view text_content) -> std::vector<Token>;

    [[nodiscard]] auto decorate_inline_text(std::string_view text_content) const -> std::string;

    [[nodiscard]] auto decorate_code_block(std::string_view code, const std::string &lang) const
        -> std::string;

    [[nodiscard]] auto emit(const std::vector<Token> &tokens) const -> std::string;

    // Decoupled Structural Handlers
    void handle_header(const Token &token_ref, std::string &html_output,
                       std::string_view size_modifier) const;

    void handle_list(const Token &token_ref, std::string &html_output, ListStatePair flags,
                     std::string_view list_tag) const;

    void handle_paragraph(const Token &token_ref, std::string &html_output) const;
    void handle_code_block(const Token &token_ref, std::string &html_output) const;

    // Fixes Issue 2: Stateless visual break layout made static
    static void handle_divider(std::string &html_output);

    auto scan_and_emit_table(const std::vector<Token> &tokens, size_t &current_idx,
                             std::string &html_output,
                             const std::function<void()> &close_active_lists) const -> bool;

    config::AppearanceConfig m_theme;
    SyntaxRegistry m_registry;
};

}  // namespace malama::engine::markdown

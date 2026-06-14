#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include "config/config_manager.hpp"

namespace malama::engine::markdown {

enum class token_type: std::uint8_t {
    paragraph,
    header_1,
    header_2,
    header_3,
    list_unordered,
    list_ordered,
    code_block,
    divider
};

struct Token {
    token_type m_type;
    std::string m_content;
    std::string m_language; // Only used for code_block
};

class Pipeline {
public:
    explicit Pipeline(config::AppearanceConfig theme);

    // The entry point for the three-stage process
    [[nodiscard]] auto process(std::string_view raw_markdown) const -> std::string;

private:
    // Stage 1: Scanner
    [[nodiscard]] static auto tokenize(std::string_view text) -> std::vector<Token>;
    
    // Stage 2: Styler & Syntax Highlighter
    [[nodiscard]] auto decorate_code_block(std::string_view code) const -> std::string;
    [[nodiscard]] auto decorate_inline_text(std::string_view text) const -> std::string;
    
    // Stage 3: Painter
    [[nodiscard]] auto emit(const std::vector<Token>& tokens) const -> std::string;

    config::AppearanceConfig m_theme;
};

} // namespace malama::engine::markdown

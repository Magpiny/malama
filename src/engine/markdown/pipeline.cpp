// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/pipeline.cpp
// Purpose:     Exception-free data processing logic with isolated control streams
// Author:      Magpiny <magpinyb@proton.me>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "engine/markdown/pipeline.hpp"
#include <string>
#include <regex>
#include <utility>

namespace malama::engine::markdown {

Pipeline::Pipeline(config::AppearanceConfig theme) noexcept 
    : m_theme(std::move(theme)) {}

auto Pipeline::process(std::string_view raw_markdown) const -> std::string {
    auto tokens = tokenize(raw_markdown);
    return emit(tokens);
}

struct TokenizeState {
    std::vector<Token> m_tokens;
    std::string m_current_buffer;
    std::string m_code_lang;
    bool m_in_code_block{false};

    void push_buffer_as(token_type type) {
        if (!m_current_buffer.empty()) {
            Token tok{.m_type = type, .m_content = m_current_buffer, .m_language = m_code_lang};
            m_tokens.push_back(tok);
            m_current_buffer.clear();
            m_code_lang.clear();
        }
    }
};

static auto process_markdown_line(std::string_view line, TokenizeState& state) -> void {
    if (line.starts_with("```")) {
        if (state.m_in_code_block) {
            state.push_buffer_as(token_type::code_block);
            state.m_in_code_block = false;
        } else {
            state.push_buffer_as(token_type::paragraph);
            state.m_in_code_block = true;
            state.m_code_lang = std::string(line.substr(3));
        }
        return;
    } 
    
    if (state.m_in_code_block) {
        state.m_current_buffer += std::string(line) + "\n";
        return; 
    } 
    
    if (line.starts_with("### ")) {
        state.push_buffer_as(token_type::paragraph);
        state.m_current_buffer = line.substr(4);
        state.push_buffer_as(token_type::header_3);
    } else if (line.starts_with("## ")) {
        state.push_buffer_as(token_type::paragraph);
        state.m_current_buffer = line.substr(3);
        state.push_buffer_as(token_type::header_2);
    } else if (line.starts_with("# ")) {
        state.push_buffer_as(token_type::paragraph);
        state.m_current_buffer = line.substr(2);
        state.push_buffer_as(token_type::header_1);
    } else if (line.starts_with("---")) {
        state.push_buffer_as(token_type::paragraph);
        state.push_buffer_as(token_type::divider);
    } else if (line.starts_with("* ") || line.starts_with("- ")) {
        state.push_buffer_as(token_type::paragraph); 
        state.m_current_buffer = line.substr(2);
        state.push_buffer_as(token_type::list_unordered);
    } else if (std::regex_search(std::string(line), std::regex(R"(^\d+\.\s)"))) {
        state.push_buffer_as(token_type::paragraph);
        size_t dot_pos = line.find('.');
        state.m_current_buffer = line.substr(dot_pos + 2);
        state.push_buffer_as(token_type::list_ordered);
    } else {
        if (line.empty()) {
            state.m_current_buffer += "<br>";
        } else {
            state.m_current_buffer += std::string(line) + " ";
        }
    }
}

auto Pipeline::tokenize(std::string_view text) -> std::vector<Token> {
    TokenizeState state;
    std::string_view remaining = text;

    while (!remaining.empty()) {
        size_t newline_pos = remaining.find('\n');
        std::string_view raw_line = (newline_pos == std::string_view::npos) 
                                  ? remaining 
                                  : remaining.substr(0, newline_pos);
        
        std::string_view line = raw_line;
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }

        if (newline_pos == std::string_view::npos) {
            remaining = "";
        } else {
            remaining.remove_prefix(newline_pos + 1);
        }

        process_markdown_line(line, state);
    }
    
    state.push_buffer_as(token_type::paragraph);
    return state.m_tokens;
}

auto Pipeline::decorate_inline_text(std::string_view text) const -> std::string {
    std::string processed{text};
    processed = std::regex_replace(processed, std::regex(R"(\*\*(.*?)\*\*)"), "<b>$1</b>");
    
    std::string inline_code_tag = "<font color=\"" + m_theme.m_code_string + 
                                  R"(" face="monospace">$1</font>)";
    processed = std::regex_replace(processed, std::regex(R"(`(.*?)`)"), inline_code_tag);
    return processed;
}

auto Pipeline::decorate_code_block(
    std::string_view code, 
    const std::string& lang
) const -> std::string {
    std::string processed{code};
    std::erase(processed, '\r');

    // 1. Encode HTML entities FIRST
    processed = std::regex_replace(processed, std::regex("&"), "&amp;");
    processed = std::regex_replace(processed, std::regex("<"), "&lt;");
    processed = std::regex_replace(processed, std::regex(">"), "&gt;");

    // 2. Map syntax rules cleanly using decoupled temporary references
    const auto* syntax = m_registry.GetSyntaxFor(lang);
    if (syntax != nullptr) {
        for (const auto& rule : syntax->m_rules) {
            const auto& pat = rule.m_compiled_pattern;
            const auto& fmt = rule.m_replacement_format;
            processed = std::regex_replace(processed, pat, fmt);
        }
    }

    // 3. Format Spacing for wxHtmlWindow (Now isolated from colors completely)
    processed = std::regex_replace(processed, std::regex("\n"), "<br>");
    processed = std::regex_replace(processed, std::regex("\t"), "&nbsp;&nbsp;&nbsp;&nbsp;");
    processed = std::regex_replace(processed, std::regex("  "), "&nbsp;&nbsp;");

    // 4. Expand Control Markers to Colors safely
    std::string t_str = "<font color=\"" + m_theme.m_code_string + "\">";
    processed = std::regex_replace(processed, std::regex("\x01"), t_str);
    processed = std::regex_replace(processed, std::regex("\x02"), "</font>");

    std::string t_com = "<font color=\"" + m_theme.m_code_comment + "\">";
    processed = std::regex_replace(processed, std::regex("\x03"), t_com);
    processed = std::regex_replace(processed, std::regex("\x04"), "</font>");

    std::string t_key = "<font color=\"" + m_theme.m_code_keyword + "\">";
    processed = std::regex_replace(processed, std::regex("\x05"), t_key);
    processed = std::regex_replace(processed, std::regex("\x06"), "</font>");

    processed = std::regex_replace(processed, std::regex("\x07"), "<font color=\"#E5C07B\">"); 
    processed = std::regex_replace(processed, std::regex("\x08"), "</font>");
    
    // Remapped expanded HTML bindings to match non-whitespace indices
    processed = std::regex_replace(processed, std::regex("\x0F"), "<font color=\"#D19A66\">"); 
    processed = std::regex_replace(processed, std::regex("\x10"), "</font>");
    processed = std::regex_replace(processed, std::regex("\x11"), "<font color=\"#98C379\">"); 
    processed = std::regex_replace(processed, std::regex("\x12"), "</font>");
    processed = std::regex_replace(processed, std::regex("\x13"), "<font color=\"#43A047\">"); 
    processed = std::regex_replace(processed, std::regex("\x14"), "</font>");

    std::string html = R"(<br><table width="100%" bgcolor=")";
    html += m_theme.m_code_bg;
    html += R"(" cellpadding="10"><tr><td valign="top"><font color=")";
    html += m_theme.m_text_primary;
    html += R"(" face="monospace">)";
    html += processed + "</font></td></tr></table><br>";
    return html;
}

auto Pipeline::emit(const std::vector<Token>& tokens) const -> std::string {
    std::string html_output;
    bool in_ul = false;
    bool in_ol = false;

    auto close_lists = [&]() {
        if (in_ul) {
            html_output += "</ul>";
            in_ul = false;
        }
        if (in_ol) {
            html_output += "</ol><br>";
            in_ol = false;
        }
    };

    for (const auto& tok : tokens) {
        if (tok.m_type != token_type::list_unordered && tok.m_type != token_type::list_ordered) {
            close_lists();
        }

        switch (tok.m_type) {
            case token_type::header_1:
                html_output += R"(<br><b><font size="+2" color=")";
                html_output += m_theme.m_text_accent;
                html_output += "\">" + decorate_inline_text(tok.m_content);
                html_output += "</font></b><br><br>";
                break;
            case token_type::header_2:
                html_output += R"(<br><b><font size="+1" color=")";
                html_output += m_theme.m_text_accent;
                html_output += "\">" + decorate_inline_text(tok.m_content);
                html_output += "</font></b><br><br>";
                break;
            case token_type::header_3:
                html_output += "<br><b><font color=\"";
                html_output += m_theme.m_text_accent;
                html_output += "\">" + decorate_inline_text(tok.m_content);
                html_output += "</font></b><br><br>";
                break;
            case token_type::divider:
                html_output += "<hr>";
                break;
            case token_type::list_unordered:
                if (!in_ul) {
                    html_output += "<ul>";
                    in_ul = true;
                }
                html_output += "<li>" + decorate_inline_text(tok.m_content) + "</li>";
                break;
            case token_type::list_ordered:
                if (!in_ol) {
                    html_output += "<ol>";
                    in_ol = true;
                }
                html_output += "<li>" + decorate_inline_text(tok.m_content) + "</li>";
                break;
            case token_type::code_block:
                html_output += decorate_code_block(tok.m_content, tok.m_language);
                break;
            case token_type::paragraph:
            default:
                if (!tok.m_content.empty()) {
                    html_output += "<p>" + decorate_inline_text(tok.m_content) + "</p>";
                }
                break;
        }
    }
    close_lists();
    return html_output;
}

} // namespace malama::engine::markdown

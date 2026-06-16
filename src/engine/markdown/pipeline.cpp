// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/pipeline.cpp
// Purpose:     Exception-free data processing logic for Markdown styling
// Author:      Magpiny <magpinyb@proton.me>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#include "engine/markdown/pipeline.hpp"
#include <string>
#include <regex>
#include <utility>

namespace malama::engine::markdown {

Pipeline::Pipeline(config::AppearanceConfig theme) 
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
            m_tokens.push_back({.m_type = type, .m_content = m_current_buffer, .m_language = m_code_lang});
            m_current_buffer.clear();
            m_code_lang.clear();
        }
    }
};

static auto process_markdown_line(std::string_view line, TokenizeState& state) -> void {
    // Guard Clause 1: Handle code block toggles immediately
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
    
    // Guard Clause 2: If we are inside a code block, append and exit
    if (state.m_in_code_block) {
        state.m_current_buffer += std::string(line) + "\n";
        return; 
    } 
    
    // Standard Markdown processing (Flat if/else chain)
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
        std::string_view raw_line = (newline_pos == std::string_view::npos) ? remaining : remaining.substr(0, newline_pos);
        
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
    
    // Bold: **text** -> <b>text</b>
    processed = std::regex_replace(processed, std::regex(R"(\*\*(.*?)\*\*)"), "<b>$1</b>");
    
    // Inline code: `text`
    std::string inline_code_tag = "<font color=\"" + m_theme.m_code_string + R"(" face="monospace">$1</font>)";
    processed = std::regex_replace(processed, std::regex(R"(`(.*?)`)"), inline_code_tag);
    
    return processed;
}

auto Pipeline::decorate_code_block(std::string_view code, const std::string& lang) const -> std::string {
    std::string processed{code};
    std::erase(processed, '\r');

    // 1. Encode HTML entities FIRST
    processed = std::regex_replace(processed, std::regex("&"), "&amp;");
    processed = std::regex_replace(processed, std::regex("<"), "&lt;");
    processed = std::regex_replace(processed, std::regex(">"), "&gt;");

    // 2. Apply Dynamic Grammar Rules
    const auto* syntax = m_registry.GetSyntaxFor(lang);
    if (syntax != nullptr) {
        for (const auto& rule : syntax->m_rules) {
            processed = std::regex_replace(processed, rule.m_pattern, rule.m_replacement);
        }
    }

    // 3. Format Spacing for wxHtmlWindow
    processed = std::regex_replace(processed, std::regex("\n"), "<br>");
    processed = std::regex_replace(processed, std::regex("\t"), "&nbsp;&nbsp;&nbsp;&nbsp;");
    processed = std::regex_replace(processed, std::regex("  "), "&nbsp;&nbsp;");

    // 4. Expand Control Markers to Colors
    // Standard Theme Colors
    processed = std::regex_replace(processed, std::regex("\x01"), "<font color=\"" + m_theme.m_code_string + "\">");
    processed = std::regex_replace(processed, std::regex("\x02"), "</font>");
    processed = std::regex_replace(processed, std::regex("\x03"), "<font color=\"" + m_theme.m_code_comment + "\">");
    processed = std::regex_replace(processed, std::regex("\x04"), "</font>");
    processed = std::regex_replace(processed, std::regex("\x05"), "<font color=\"" + m_theme.m_code_keyword + "\">");
    processed = std::regex_replace(processed, std::regex("\x06"), "</font>");

    // Custom Specific Semantic Colors
    // Entity (Classes, Enums, Funcs) -> Yellow
    processed = std::regex_replace(processed, std::regex("\x07"), "<font color=\"#E5C07B\">"); 
    processed = std::regex_replace(processed, std::regex("\x08"), "</font>");
    
    // Punctuation (Braces) -> Orange
    processed = std::regex_replace(processed, std::regex("\x09"), "<font color=\"#D19A66\">"); 
    processed = std::regex_replace(processed, std::regex("\x0A"), "</font>");
    
    // Headers/Includes -> Green
    processed = std::regex_replace(processed, std::regex("\x0B"), "<font color=\"#98C379\">"); 
    processed = std::regex_replace(processed, std::regex("\x0C"), "</font>");
    
    // Methods (namespace::class::method) -> Dark Green
    processed = std::regex_replace(processed, std::regex("\x0D"), "<font color=\"#43A047\">"); 
    processed = std::regex_replace(processed, std::regex("\x0E"), "</font>");

    return R"(<br><table width="100%" bgcolor=")" + m_theme.m_code_bg + "\" cellpadding=\"10\">"
           "<tr><td valign=\"top\"><font color=\"" + m_theme.m_text_primary + R"(" face="monospace">)" + processed + "</font></td></tr>"
           "</table><br>";
}

auto Pipeline::emit(const std::vector<Token>& tokens) const -> std::string {
    std::string html_output;
    bool in_ul = false;
    bool in_ol = false;

    auto close_lists = [&]() {
        if (in_ul) { html_output += "</ul>"; in_ul = false; }
        if (in_ol) { html_output += "</ol><br>"; in_ol = false; }
    };

    for (const auto& tok : tokens) {
        if (tok.m_type != token_type::list_unordered && tok.m_type != token_type::list_ordered) {
            close_lists();
        }

        switch (tok.m_type) {
            case token_type::header_1:
                html_output += R"(<br><b><font size="+2" color=")" + m_theme.m_text_accent + "\">" + decorate_inline_text(tok.m_content) + "</font></b><br><br>";
                break;
            case token_type::header_2:
                html_output += R"(<br><b><font size="+1" color=")" + m_theme.m_text_accent + "\">" + decorate_inline_text(tok.m_content) + "</font></b><br><br>";
                break;
            case token_type::header_3:
                html_output += "<br><b><font color=\"" + m_theme.m_text_accent + "\">" + decorate_inline_text(tok.m_content) + "</font></b><br><br>";
                break;
            case token_type::divider:
                html_output += "<hr>";
                break;
            case token_type::list_unordered:
                if (!in_ul) { html_output += "<ul>"; in_ul = true; }
                html_output += "<li>" + decorate_inline_text(tok.m_content) + "</li>";
                break;
            case token_type::list_ordered:
                if (!in_ol) { html_output += "<ol>"; in_ol = true; }
                html_output += "<li>" + decorate_inline_text(tok.m_content) + "</li>";
                break;
            case token_type::code_block:
                html_output += decorate_code_block(tok.m_content,tok.m_language);
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

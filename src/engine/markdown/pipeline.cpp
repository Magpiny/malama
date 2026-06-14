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
#include <sstream>
#include <regex>
#include <utility>

namespace malama::engine::markdown {

Pipeline::Pipeline(config::AppearanceConfig theme) 
    : m_theme(std::move(theme)) {}

auto Pipeline::process(std::string_view raw_markdown) const -> std::string {
    auto tokens = tokenize(raw_markdown);
    return emit(tokens);
}

auto Pipeline::tokenize(std::string_view text) -> std::vector<Token> {
    std::vector<Token> tokens;
    std::string current_buffer;
    std::string_view remaining = text;
    bool in_code_block = false;
    std::string code_lang;

    auto push_buffer_as = [&](token_type type) {
        if (!current_buffer.empty()) {
            tokens.push_back({.m_type=type, .m_content=current_buffer, .m_language=code_lang});
            current_buffer.clear();
            code_lang.clear();
        }
    };

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

        if (line.starts_with("```")) {
            if (in_code_block) {
                push_buffer_as(token_type::code_block);
                in_code_block = false;
            } else {
                push_buffer_as(token_type::paragraph);
                in_code_block = true;
                code_lang = std::string(line.substr(3));
            }
        } else if (in_code_block) {
            current_buffer += std::string(line) + "\n";
        } else if (line.starts_with("### ")) {
            push_buffer_as(token_type::paragraph);
            current_buffer = line.substr(4);
            push_buffer_as(token_type::header_3);
        } else if (line.starts_with("## ")) {
            push_buffer_as(token_type::paragraph);
            current_buffer = line.substr(3);
            push_buffer_as(token_type::header_2);
        } else if (line.starts_with("# ")) {
            push_buffer_as(token_type::paragraph);
            current_buffer = line.substr(2);
            push_buffer_as(token_type::header_1);
        } else if (line.starts_with("---")) {
            push_buffer_as(token_type::paragraph);
            push_buffer_as(token_type::divider);
        } else if (line.starts_with("* ") || line.starts_with("- ")) {
            push_buffer_as(token_type::paragraph); 
            current_buffer = line.substr(2);
            push_buffer_as(token_type::list_unordered);
        } else if (std::regex_search(std::string(line), std::regex(R"(^\d+\.\s)"))) {
            push_buffer_as(token_type::paragraph);
            size_t dot_pos = line.find('.');
            current_buffer = line.substr(dot_pos + 2);
            push_buffer_as(token_type::list_ordered);
        } else {
            if (line.empty()) {
                current_buffer += "<br>";
            } else {
                current_buffer += std::string(line) + " ";
            }
        }
    }
    push_buffer_as(token_type::paragraph);
    return tokens;
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

auto Pipeline::decorate_code_block(std::string_view code) const -> std::string {
    std::string processed{code};

    // 1. Strip trailing carriage returns natively
    std::erase(processed, '\r');

    // 2. Encode HTML entities to prevent rendering breaks
    processed = std::regex_replace(processed, std::regex("&"), "&amp;");
    processed = std::regex_replace(processed, std::regex("<"), "&lt;");
    processed = std::regex_replace(processed, std::regex(">"), "&gt;");

    // 3. Highlight Syntax
    std::regex keyword_rx(R"(\b(def|class|return|if|else|elif|for|while|import|from|int|void|auto|const|std|cout|endl|string|vector|namespace|public|private|protected)\b)");
    std::regex preproc_rx(R"((#\s*[a-zA-Z]+))");
    std::regex string_rx(R"((\"[^\"]*\"))");
    std::regex comment_rx(R"((//.*))");

    processed = std::regex_replace(processed, preproc_rx, "<font color=\"" + m_theme.m_code_keyword + "\">$1</font>");
    processed = std::regex_replace(processed, keyword_rx, "<font color=\"" + m_theme.m_code_keyword + "\">$1</font>");
    processed = std::regex_replace(processed, string_rx, "<font color=\"" + m_theme.m_code_string + "\">$1</font>");
    processed = std::regex_replace(processed, comment_rx, "<font color=\"" + m_theme.m_code_comment + "\">$1</font>");

    // 4. Wrap in <pre> tag inside a simple background table. <pre> handles newlines natively!
    return R"(<br><table width="100%" bgcolor=")" + m_theme.m_code_bg + "\" cellpadding=\"10\">"
           "<tr><td><pre><font color=\"" + m_theme.m_text_primary + "\">" + processed + "</font></pre></td></tr>"
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
                html_output += decorate_code_block(tok.m_content);
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

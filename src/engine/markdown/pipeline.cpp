// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/pipeline.cpp
// Purpose:     Decoupled zero-allocation markdown pipeline with safe std::array
// Author:      Wanjare S. <samuewanjare@protonmail.com>
// Created:     2026-07-01
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "engine/markdown/pipeline.hpp"
#include <boost/regex.hpp>
#include <boost/spirit/home/x3.hpp>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <array>

namespace malama::engine::markdown {

namespace x3 = boost::spirit::x3;

Pipeline::Pipeline(config::AppearanceConfig theme) noexcept 
    : m_theme(std::move(theme)) {}

auto Pipeline::process(std::string_view raw_markdown) const -> std::string {
    auto tokens = tokenize(raw_markdown);
    return emit(tokens);
}

struct TokenizerState {
    std::vector<Token> m_tokens;
    std::string m_buffer;
    std::string m_language;
    bool m_in_block{false};

    void push_buffer(token_type type) {
        if (!m_buffer.empty()) {
            Token token;
            token.m_type = type;
            token.m_content = m_buffer;
            token.m_language = m_language;
            m_tokens.push_back(token);
            m_buffer.clear();
            m_language.clear();
        }
    }
};

static void parse_markdown_elements(
    std::string_view line_view,
    TokenizerState& state
) {
    auto head_3_tag = x3::lit("### ");
    auto head_2_tag = x3::lit("## ");
    auto head_1_tag = x3::lit("# ");
    auto divider_tag = x3::lit("---");
    auto list_un_tag = x3::lit("* ") | x3::lit("- ");
    auto list_or_tag = x3::lexeme[+x3::digit >> x3::lit(". ")];

    auto line_start = line_view.cbegin();
    auto line_final = line_view.cend();

    if (x3::parse(line_start, line_final, head_3_tag)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_start, line_final);
        state.push_buffer(token_type::header_3);
    } else if (x3::parse(line_start, line_final, head_2_tag)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_start, line_final);
        state.push_buffer(token_type::header_2);
    } else if (x3::parse(line_start, line_final, head_1_tag)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_start, line_final);
        state.push_buffer(token_type::header_1);
    } else if (x3::parse(line_start, line_final, divider_tag)) {
        state.push_buffer(token_type::paragraph);
        state.push_buffer(token_type::divider);
    } else if (x3::parse(line_start, line_final, list_un_tag)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_start, line_final);
        state.push_buffer(token_type::list_unordered);
    } else if (x3::parse(line_start, line_final, list_or_tag)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_start, line_final);
        state.push_buffer(token_type::list_ordered);
    } else {
        if (line_view.empty()) {
            state.m_buffer += "<br>";
        } else {
            state.m_buffer += std::string(line_view) + " ";
        }
    }
}

static void evaluate_line_tokens(
    std::string_view line_view, 
    TokenizerState& state
) {
    auto block_tag = x3::lit("```");
    auto line_start = line_view.cbegin();
    auto line_final = line_view.cend();

    if (x3::parse(line_start, line_final, block_tag)) {
        if (state.m_in_block) {
            state.push_buffer(token_type::code_block);
            state.m_in_block = false;
        } else {
            state.push_buffer(token_type::paragraph);
            state.m_in_block = true;
            state.m_language = std::string(line_start, line_final);
        }
        return;
    }

    if (state.m_in_block) {
        state.m_buffer += std::string(line_view) + "\n";
        return;
    }

    parse_markdown_elements(line_view, state);
}

auto Pipeline::tokenize(std::string_view text) -> std::vector<Token> {
    TokenizerState state;
    auto start_iter = text.cbegin();
    auto final_iter = text.cend();

    while (start_iter != final_iter) {
        auto end_line_iter = std::find(start_iter, final_iter, '\n');
        std::string_view line_view(&*start_iter, std::distance(start_iter, end_line_iter));
        
        if (!line_view.empty() && line_view.back() == '\r') {
            line_view.remove_suffix(1);
        }

        evaluate_line_tokens(line_view, state);

        if (end_line_iter == final_iter) {
            break;
        }
        start_iter = end_line_iter + 1;
    }

    state.push_buffer(token_type::paragraph);
    return state.m_tokens;
}

auto Pipeline::decorate_inline_text(std::string_view text) const -> std::string {
    static const boost::regex bold_pattern(R"(\*\*(.*?)\*\*)");
    static const boost::regex code_pattern(R"(`(.*?)`)");

    std::string processed{text};
    processed = boost::regex_replace(processed, bold_pattern, "<b>$1</b>");
    
    std::string inline_code_tag = "<font color=\"" + m_theme.m_code_string + 
                                  R"(" face="monospace">$1</font>)";
    processed = boost::regex_replace(processed, code_pattern, inline_code_tag);
    return processed;
}

auto Pipeline::decorate_code_block(
    std::string_view code, 
    const std::string& lang
) const -> std::string {
    static const boost::regex entity_amp("&");
    static const boost::regex entity_lt("<");
    static const boost::regex entity_gt(">");
    static const boost::regex format_nl("\n");
    static const boost::regex format_tab("\t");
    static const boost::regex format_sp("  ");
    static const boost::regex marker_01("\x01");
    static const boost::regex marker_02("\x02");
    static const boost::regex marker_03("\x03");
    static const boost::regex marker_04("\x04");
    static const boost::regex marker_05("\x05");
    static const boost::regex marker_06("\x06");
    static const boost::regex marker_07("\x07");
    static const boost::regex marker_08("\x08");
    static const boost::regex marker_0F("\x0F");
    static const boost::regex marker_10("\x10");
    static const boost::regex marker_11("\x11");
    static const boost::regex marker_12("\x12");
    static const boost::regex marker_13("\x13");
    static const boost::regex marker_14("\x14");

    std::string processed{code};
    std::erase(processed, '\r');

    processed = boost::regex_replace(processed, entity_amp, "&amp;");
    processed = boost::regex_replace(processed, entity_lt, "&lt;");
    processed = boost::regex_replace(processed, entity_gt, "&gt;");

    const auto* syntax = m_registry.GetSyntaxFor(lang);
    if (syntax != nullptr) {
        std::string result_string;
        size_t current_pos = 0;
        while (current_pos < processed.size()) {
            bool found_any = false;
            size_t best_start = processed.size();
            size_t best_final = current_pos;
            size_t best_rule_idx = 0;
            boost::smatch best_match;

            for (size_t rule_idx = 0; rule_idx < syntax->m_rules.size(); ++rule_idx) {
                boost::smatch match_results;
                auto start_iter = processed.cbegin() + current_pos;
                auto final_iter = processed.cend();
                const auto& rule_ref = syntax->m_rules[rule_idx];
                if (boost::regex_search(start_iter, final_iter, match_results, 
                    rule_ref.m_compiled_pattern)) {
                    size_t match_start = current_pos + match_results.position();
                    size_t match_final = match_start + match_results.length();
                    if (match_start < best_start) {
                        best_start = match_start;
                        best_final = match_final;
                        best_rule_idx = rule_idx;
                        best_match = match_results;
                        found_any = true;
                    }
                }
            }

            if (!found_any) {
                result_string += processed.substr(current_pos);
                break;
            }

            result_string += processed.substr(current_pos, best_start - current_pos);
            const auto& rule_ref = syntax->m_rules[best_rule_idx];
            result_string += best_match.format(rule_ref.m_replacement_format);
            current_pos = (best_final == current_pos) ? current_pos + 1 : best_final;
        }
        processed = result_string;
    }

    processed = boost::regex_replace(processed, format_nl, "<br>");
    processed = boost::regex_replace(processed, format_tab, "&nbsp;&nbsp;&nbsp;&nbsp;");
    processed = boost::regex_replace(processed, format_sp, "&nbsp;&nbsp;");

    std::string text_color_tag = "<font color=\"" + m_theme.m_code_string + "\">";
    processed = boost::regex_replace(processed, marker_01, text_color_tag);
    processed = boost::regex_replace(processed, marker_02, "</font>");

    std::string comment_color_tag = "<font color=\"" + m_theme.m_code_comment + "\">";
    processed = boost::regex_replace(processed, marker_03, comment_color_tag);
    processed = boost::regex_replace(processed, marker_04, "</font>");

    std::string keyword_color_tag = "<font color=\"" + m_theme.m_code_keyword + "\">";
    processed = boost::regex_replace(processed, marker_05, keyword_color_tag);
    processed = boost::regex_replace(processed, marker_06, "</font>");

    processed = boost::regex_replace(processed, marker_07, "<font color=\"#E5C07B\">"); 
    processed = boost::regex_replace(processed, marker_08, "</font>");
    processed = boost::regex_replace(processed, marker_0F, "<font color=\"#D19A66\">"); 
    processed = boost::regex_replace(processed, marker_10, "</font>");
    processed = boost::regex_replace(processed, marker_11, "<font color=\"#98C379\">"); 
    processed = boost::regex_replace(processed, marker_12, "</font>");
    processed = boost::regex_replace(processed, marker_13, "<font color=\"#43A047\">"); 
    processed = boost::regex_replace(processed, marker_14, "</font>");

    std::string hex_encoded_string;
    hex_encoded_string.reserve(code.size() * 2);

    // Replaced standard C-style array format with standard type-safe std::array layout
    static const std::array<char, 16> hex_digits{
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };
    for (char character_value : code) {
        hex_encoded_string.push_back(hex_digits[(character_value >> 4) & 0x0F]);
        hex_encoded_string.push_back(hex_digits[character_value & 0x0F]);
    }

    std::string actions_html = R"(<div align="right">)";
    actions_html += R"(<a href="malama://copy_code:)";
    actions_html += hex_encoded_string;
    actions_html += R"(" title="Copy"><font size="-1">&#x1F4CB;</font></a>)";
    actions_html += R"(&nbsp;&nbsp;)";
    actions_html += R"(<a href="malama://download_code:)";
    actions_html += hex_encoded_string;
    actions_html += R"(" title="Download"><font size="-1">&#x1F4E5;</font></a>)";
    actions_html += R"(</div>)";

    std::string html_output = R"(<br><table width="100%" bgcolor=")";
    html_output += m_theme.m_code_bg;
    html_output += R"(" cellpadding="10"><tr><td valign="top"><font color=")";
    html_output += m_theme.m_text_primary;
    html_output += R"(" face="monospace">)";
    html_output += processed + "</font>" + actions_html + "</td></tr></table><br>";
    return html_output;
}

auto Pipeline::emit(const std::vector<Token>& tokens) const -> std::string {
    std::string html_output;
    bool in_unordered_list = false;
    bool in_ordered_list = false;

    auto close_lists = [&]() {
        if (in_unordered_list) {
            html_output += "</ul>";
            in_unordered_list = false;
        }
        if (in_ordered_list) {
            html_output += "</ol><br>";
            in_ordered_list = false;
        }
    };

    for (const auto& token_ref : tokens) {
        bool is_ul = (token_ref.m_type == token_type::list_unordered);
        bool is_ol = (token_ref.m_type == token_type::list_ordered);
        if (!is_ul && !is_ol) {
            close_lists();
        }

        switch (token_ref.m_type) {
            case token_type::header_1:
                html_output += R"(<br><b><font size="+2" color=")";
                html_output += m_theme.m_text_accent;
                html_output += "\">" + decorate_inline_text(token_ref.m_content);
                html_output += "</font></b><br><br>";
                break;
            case token_type::header_2:
                html_output += R"(<br><b><font size="+1" color=")";
                html_output += m_theme.m_text_accent;
                html_output += "\">" + decorate_inline_text(token_ref.m_content);
                html_output += "</font></b><br><br>";
                break;
            case token_type::header_3:
                html_output += "<br><b><font color=\"";
                html_output += m_theme.m_text_accent;
                html_output += "\">" + decorate_inline_text(token_ref.m_content);
                html_output += "</font></b><br><br>";
                break;
            case token_type::divider:
                html_output += "<hr>";
                break;
            case token_type::list_unordered:
                if (!in_unordered_list) {
                    html_output += "<ul>";
                    in_unordered_list = true;
                }
                html_output += "<li>" + decorate_inline_text(token_ref.m_content) + "</li>";
                break;
            case token_type::list_ordered:
                if (!in_ordered_list) {
                    html_output += "<ol>";
                    in_ordered_list = true;
                }
                html_output += "<li>" + decorate_inline_text(token_ref.m_content) + "</li>";
                break;
            case token_type::code_block:
                html_output += decorate_code_block(token_ref.m_content, token_ref.m_language);
                break;
            case token_type::paragraph:
            default:
                if (!token_ref.m_content.empty()) {
                    html_output += "<p>" + decorate_inline_text(token_ref.m_content) + "</p>";
                }
                break;
        }
    }
    close_lists();
    return html_output;
}

} // namespace malama::engine::markdown

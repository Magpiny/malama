// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/pipeline.cpp
// Purpose:     Decoupled zero-allocation markdown pipeline with safe std::array
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-01
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "engine/markdown/pipeline.hpp"

#include <algorithm>
#include <array>
#include <boost/regex.hpp>
#include <boost/spirit/home/x3.hpp>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace malama::engine::markdown {

namespace x3 = boost::spirit::x3;

namespace local_constants {
constexpr size_t hex_buffer_reserve_scale = 2;
constexpr size_t hex_bits_shift_value = 4;
constexpr char hex_mask_lower_nibble = 0x0F;
constexpr size_t step_past_delimiter = 1;
constexpr size_t structural_zero_index = 0;
constexpr size_t minimum_valid_table_rows = 2;

constexpr std::string_view break_line_marker = "<br>";
constexpr std::string_view spaces_accumulation = " ";
constexpr std::string_view horizontal_rule_tag = "<hr>";
constexpr std::string_view paragraph_start_tag = "<p>";
constexpr std::string_view paragraph_close_tag = "</p>";
constexpr std::string_view unordered_list_tag = "ul";
constexpr std::string_view ordered_list_tag = "ol";
constexpr std::string_view header_1_size = "+2";
constexpr std::string_view header_2_size = "+1";
constexpr std::string_view header_3_size;
}  // namespace local_constants

Pipeline::Pipeline(config::AppearanceConfig theme) noexcept : m_theme(std::move(theme)) {}

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

static void parse_markdown_elements(std::string_view line_view, TokenizerState &state) {
    auto head_3_tag = x3::lit("### ");
    auto head_2_tag = x3::lit("## ");
    auto head_1_tag = x3::lit("# ");
    auto divider_tag = x3::lit("---");
    auto list_un_tag = x3::lit("* ") | x3::lit("- ");
    auto list_or_tag = x3::lexeme[+x3::digit >> x3::lit(". ")];

    const auto *line_start = line_view.cbegin();
    const auto *line_final = line_view.cend();

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
        if (!line_view.empty() && line_view.front() == '|') {
            state.push_buffer(token_type::paragraph);
            Token table_token;
            table_token.m_type = token_type::paragraph;
            table_token.m_content = std::string(line_view);
            state.m_tokens.push_back(table_token);
        } else if (line_view.empty()) {
            state.m_buffer += local_constants::break_line_marker;
        } else {
            state.m_buffer +=
                std::string(line_view) + std::string(local_constants::spaces_accumulation);
        }
    }
}

static void evaluate_line_tokens(std::string_view line_view, TokenizerState &state) {
    auto block_tag = x3::lit("```");
    const auto *line_start = line_view.cbegin();
    const auto *line_final = line_view.cend();

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

auto Pipeline::tokenize(std::string_view text_content) -> std::vector<Token> {
    TokenizerState state;
    size_t current_position = local_constants::structural_zero_index;
    const size_t total_length = text_content.size();

    while (current_position < total_length) {
        const size_t newline_position = text_content.find('\n', current_position);

        const size_t segment_length = (newline_position == std::string_view::npos)
                                          ? total_length - current_position
                                          : newline_position - current_position;

        std::string_view line_view = text_content.substr(current_position, segment_length);

        if (!line_view.empty() && line_view.back() == '\r') {
            line_view.remove_suffix(local_constants::step_past_delimiter);
        }

        evaluate_line_tokens(line_view, state);

        if (newline_position == std::string_view::npos) {
            break;
        }

        current_position = newline_position + local_constants::step_past_delimiter;
    }

    state.push_buffer(token_type::paragraph);
    return state.m_tokens;
}

auto Pipeline::decorate_inline_text(std::string_view text_content) const -> std::string {
    static const boost::regex bold_pattern(R"(\*\*(.*?)\*\*)");
    static const boost::regex code_pattern(R"(`(.*?)`)");

    std::string processed{text_content};
    processed = boost::regex_replace(processed, bold_pattern, "<b>$1</b>");

    std::string inline_code_tag =
        "<font color=\"" + m_theme.m_code_string + R"(" face="monospace">$1</font>)";
    processed = boost::regex_replace(processed, code_pattern, inline_code_tag);
    return processed;
}

auto Pipeline::decorate_code_block(std::string_view code, const std::string &lang) const
    -> std::string {
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

    const auto *syntax = m_registry.GetSyntaxFor(lang);
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

                // Fixes Issue 5: Uses explicit static cast to avoid narrowing conversion warnings
                auto start_offset = static_cast<std::ptrdiff_t>(current_pos);
                auto start_iter = std::next(processed.cbegin(), start_offset);
                auto final_iter = processed.cend();

                const auto &rule_ref = syntax->m_rules[rule_idx];
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
            const auto &rule_ref = syntax->m_rules[best_rule_idx];
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
    hex_encoded_string.reserve(code.size() * local_constants::hex_buffer_reserve_scale);

    static const std::array<char, 16> hex_digits{'0', '1', '2', '3', '4', '5', '6', '7',
                                                 '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    for (char xchar : code) {
        const auto token_byte = static_cast<std::uint8_t>(xchar);
        hex_encoded_string.push_back(
            hex_digits.at(token_byte >> local_constants::hex_bits_shift_value));
        hex_encoded_string.push_back(
            hex_digits.at(token_byte & local_constants::hex_mask_lower_nibble));
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

void Pipeline::handle_header(const Token &token_ref, std::string &html_output,
                             std::string_view size_modifier) const {
    html_output += R"(<br><b><font )";
    if (!size_modifier.empty()) {
        html_output += "size=\"" + std::string(size_modifier) + "\" ";
    }
    html_output +=
        "color=\"" + m_theme.m_text_accent + "\">" + decorate_inline_text(token_ref.m_content);
    html_output += "</font></b><br><br>";
}

void Pipeline::handle_list(const Token &token_ref, std::string &html_output, ListStatePair flags,
                           std::string_view list_tag) const {
    // .get() returns the underlying raw bool reference seamlessly
    if (flags.m_is_alternative.get()) {
        html_output += (list_tag == local_constants::unordered_list_tag) ? "</ol><br>" : "</ul>";
        flags.m_is_alternative.get() = false;
    }
    if (!flags.m_is_active.get()) {
        html_output += "<" + std::string(list_tag) + ">";
        flags.m_is_active.get() = true;
    }
    html_output += "<li>" + decorate_inline_text(token_ref.m_content) + "</li>";
}

void Pipeline::handle_paragraph(const Token &token_ref, std::string &html_output) const {
    if (!token_ref.m_content.empty()) {
        html_output += std::string(local_constants::paragraph_start_tag) +
                       decorate_inline_text(token_ref.m_content) +
                       std::string(local_constants::paragraph_close_tag);
    }
}

void Pipeline::handle_code_block(const Token &token_ref, std::string &html_output) const {
    html_output += decorate_code_block(token_ref.m_content, token_ref.m_language);
}

// Fixes Issue 2: Static handler method implementation
void Pipeline::handle_divider(std::string &html_output) {
    html_output += local_constants::horizontal_rule_tag;
}

static auto parse_row_cells(std::string_view row_text) -> std::vector<std::string> {
    std::vector<std::string> parsed_cells;
    size_t start_pos = (row_text.front() == '|') ? 1 : 0;
    size_t end_pos = (row_text.back() == '|') ? row_text.size() - 1 : row_text.size();

    std::string current_cell;
    for (size_t char_idx = start_pos; char_idx < end_pos; ++char_idx) {
        if (row_text[char_idx] == '|') {
            parsed_cells.push_back(current_cell);
            current_cell.clear();
        } else {
            current_cell.push_back(row_text[char_idx]);
        }
    }
    parsed_cells.push_back(current_cell);
    return parsed_cells;
}

void emit_rendered_cells(std::string &html_output, const std::vector<std::string> &cells,
                         size_t row_idx, const config::AppearanceConfig &theme,
                         const std::function<std::string(std::string_view)> &decorator) {
    auto is_space = [](unsigned char character) { return std::isspace(character); };

    for (const auto &cell_content : cells) {
        std::string trimmed = cell_content;
        trimmed.erase(trimmed.begin(), std::ranges::find_if_not(trimmed, is_space));
        trimmed.erase(std::ranges::find_if_not(std::views::reverse(trimmed), is_space).base(),
                      trimmed.end());

        if (row_idx == 0) {
            html_output += R"(<th bgcolor=" )" + theme.m_surface_color + R"( "><b><font color=" )" +
                           theme.m_text_accent + R"( ">)" + decorator(trimmed) +
                           R"(</font></b></th>)";
        } else {
            html_output += R"(<td><font color=" )" + theme.m_text_primary + R"( ">)" +
                           decorator(trimmed) + R"(</font></td>)";
        }
    }
}

auto Pipeline::scan_and_emit_table(const std::vector<Token> &tokens, size_t &current_idx,
                                   std::string &html_output) const -> bool {
    const auto &token_ref = tokens[current_idx];
    if (token_ref.m_type != token_type::paragraph || token_ref.m_content.empty() ||
        token_ref.m_content.front() != '|') {
        return false;
    }

    std::vector<std::string_view> table_rows;
    size_t lookahead_idx = current_idx;

    while (lookahead_idx < tokens.size()) {
        const auto &future_token = tokens[lookahead_idx];
        if (future_token.m_type == token_type::paragraph && !future_token.m_content.empty() &&
            future_token.m_content.front() == '|') {
            table_rows.push_back(future_token.m_content);
            ++lookahead_idx;
        } else {
            break;
        }
    }

    if (table_rows.size() < local_constants::minimum_valid_table_rows) {
        return false;
    }

    html_output += R"(<br><table width="100%" border="1" cellspacing="0" cellpadding="6" )";
    html_output +=
        R"(style="border-collapse:collapse; background-color:)" + m_theme.m_code_bg + R"(;">)";

    auto decorator_wrapper = [this](std::string_view text) { return decorate_inline_text(text); };

    for (size_t row_idx = 0; row_idx < table_rows.size(); ++row_idx) {
        auto row_text = table_rows[row_idx];
        bool is_separator = std::ranges::all_of(row_text, [](char symbol) {
            return symbol == '|' || symbol == '-' || symbol == ':' || symbol == ' ' ||
                   symbol == '\t';
        });

        if (is_separator) {
            continue;
        }

        html_output += "<tr>";
        std::vector<std::string> cells = parse_row_cells(row_text);
        emit_rendered_cells(html_output, cells, row_idx, m_theme, decorator_wrapper);
        html_output += "</tr>";
    }

    html_output += "</table><br>";
    current_idx = lookahead_idx;
    return true;
}

// -----------------------------------------------------------------------------
auto Pipeline::emit(const std::vector<Token> &tokens) const -> std::string {
    std::string html_output;
    bool in_unordered_list = false;
    bool in_ordered_list = false;

    auto close_active_lists = [&]() {
        if (in_unordered_list) {
            html_output += "</ul>";
            in_unordered_list = false;
        }
        if (in_ordered_list) {
            html_output += "</ol><br>";
            in_ordered_list = false;
        }
    };

    size_t token_idx = 0;
    while (token_idx < tokens.size()) {
        if (scan_and_emit_table(tokens, token_idx, html_output)) {
            close_active_lists();
            continue;
        }

        const auto &token_ref = tokens[token_idx];

        if (token_ref.m_type != token_type::list_unordered &&
            token_ref.m_type != token_type::list_ordered) {
            close_active_lists();
        }
        switch (token_ref.m_type) {
            case token_type::header_1:
                handle_header(token_ref, html_output, local_constants::header_1_size);
                break;
            case token_type::header_2:
                handle_header(token_ref, html_output, local_constants::header_2_size);
                break;
            case token_type::header_3:
                handle_header(token_ref, html_output, local_constants::header_3_size);
                break;
            case token_type::divider:
                handle_divider(html_output);
                break;
            case token_type::list_unordered:
                // std::ref explicitly binds the local variable state safely to the wrapper
                handle_list(token_ref, html_output,
                            ListStatePair{.m_is_active = std::ref(in_unordered_list),
                                          .m_is_alternative = std::ref(in_ordered_list)},
                            local_constants::unordered_list_tag);
                break;
            case token_type::list_ordered:
                handle_list(token_ref, html_output,
                            ListStatePair{.m_is_active = std::ref(in_ordered_list),
                                          .m_is_alternative = std::ref(in_unordered_list)},
                            local_constants::ordered_list_tag);
                break;
            case token_type::code_block:
                handle_code_block(token_ref, html_output);
                break;
            case token_type::paragraph:
            default:
                handle_paragraph(token_ref, html_output);
                break;
        }
        ++token_idx;
    }

    close_active_lists();
    return html_output;
}

}  // namespace malama::engine::markdown

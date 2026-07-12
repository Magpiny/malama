// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/pipeline.cpp
// Purpose:     Markdown-to-HTML rendering pipeline with a bounded fragment
//              cache to avoid re-running regex-heavy decoration work on
//              historical tokens that have not changed since their last render.
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
#include <cctype>
#include <cstdint>
#include <list>
#include <mutex>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace malama::engine::markdown {

namespace local_constants {
constexpr size_t hex_buffer_reserve_scale = 2;
constexpr size_t hex_bits_shift_value = 4;
constexpr char hex_mask_lower_nibble = 0x0F;
constexpr size_t step_past_delimiter = 1;
constexpr size_t structural_zero_index = 0;
constexpr size_t minimum_valid_table_rows = 2;
constexpr size_t minimum_divider_length = 3;

constexpr std::string_view break_line_marker = "<br>";
constexpr std::string_view spaces_accumulation = " ";
constexpr std::string_view horizontal_rule_tag = "<hr>";
constexpr std::string_view paragraph_start_tag = "<p>";
constexpr std::string_view paragraph_close_tag = "</p>";
constexpr std::string_view unordered_list_tag = "ul";
constexpr std::string_view ordered_list_tag = "ol";
constexpr std::string_view header_1_size = "+2";
constexpr std::string_view header_2_size = "+1";
constexpr std::string_view header_3_size{};
constexpr std::string_view header_4_size{};

constexpr std::string_view fence_tag = "```";
constexpr std::string_view head_4_tag = "#### ";
constexpr std::string_view head_3_tag = "### ";
constexpr std::string_view head_2_tag = "## ";
constexpr std::string_view head_1_tag = "# ";
constexpr std::string_view list_star_tag = "* ";
constexpr std::string_view list_dash_tag = "- ";
}  // namespace local_constants

namespace {

// ---------------------------------------------------------------------------
// Fragment cache: keyed by a hash of (token content + language + active
// theme), storing the already-decorated HTML for that exact input. This is a
// translation-unit-local detail -- Pipeline's public interface in the header
// is untouched. It exists because decorate_inline_text/decorate_code_block
// are regex-heavy, and chat_panel.cpp re-runs the whole markdown pipeline
// over the full conversation history on every render; without this cache,
// every historical header/paragraph/code-block gets fully re-decorated on
// every token, which is the dominant cost in long conversations.
//
// Bounded to `max_entries` with simple LRU eviction so memory use cannot grow
// without limit over a long-running session -- consistent with the low
// memory footprint goal for this project.
class FragmentCache {
   public:
    static constexpr std::size_t max_entries = 512;

    [[nodiscard]] auto get(std::size_t key) -> std::optional<std::string> {
        const std::scoped_lock lock(m_mutex);
        auto entry_it = m_map.find(key);
        if (entry_it == m_map.end()) {
            return std::nullopt;
        }
        touch(entry_it->second.m_order_it, key);
        return entry_it->second.m_html;
    }

    void put(std::size_t key, std::string html) {
        const std::scoped_lock lock(m_mutex);
        if (m_map.contains(key)) {
            return;
        }
        if (m_map.size() >= max_entries) {
            evict_oldest();
        }
        m_order.push_back(key);
        m_map.emplace(key, Entry{std::move(html), std::prev(m_order.end())});
    }

   private:
    struct Entry {
        std::string m_html;
        std::list<std::size_t>::iterator m_order_it;
    };

    void touch(std::list<std::size_t>::iterator &order_it, std::size_t key) {
        m_order.erase(order_it);
        m_order.push_back(key);
        order_it = std::prev(m_order.end());
    }

    void evict_oldest() {
        if (m_order.empty()) {
            return;
        }
        const std::size_t oldest_key = m_order.front();
        m_order.pop_front();
        m_map.erase(oldest_key);
    }

    std::unordered_map<std::size_t, Entry> m_map;
    std::list<std::size_t> m_order;
    std::mutex m_mutex;
};

[[nodiscard]] auto inline_text_cache() -> FragmentCache & {
    static FragmentCache cache;
    return cache;
}

[[nodiscard]] auto code_block_cache() -> FragmentCache & {
    static FragmentCache cache;
    return cache;
}

inline constexpr std::size_t fnv_offset_basis = 14695981039346656037ULL;
inline constexpr std::size_t fnv_prime = 1099511628211ULL;

// FNV-1a. Fast, deterministic, adequate as a cache key -- not used for any
// security-sensitive purpose, so no cryptographic hash is needed here.
[[nodiscard]] auto hash_with_seed(std::string_view text, std::size_t seed) noexcept -> std::size_t {
    std::size_t hash_value = seed;
    for (const char byte_char : text) {
        const auto byte_value = static_cast<unsigned char>(byte_char);
        hash_value ^= byte_value;
        hash_value *= fnv_prime;
    }
    return hash_value;
}

// Cheap fingerprint of the theme fields that actually affect decoration
// output, used so a theme switch correctly invalidates cached fragments
// instead of serving stale colors.
[[nodiscard]] auto theme_fingerprint(const config::AppearanceConfig &theme) -> std::string {
    std::string fingerprint;
    fingerprint += theme.m_code_string;
    fingerprint += '\x1f';
    fingerprint += theme.m_code_comment;
    fingerprint += '\x1f';
    fingerprint += theme.m_code_keyword;
    fingerprint += '\x1f';
    fingerprint += theme.m_code_bg;
    fingerprint += '\x1f';
    fingerprint += theme.m_text_primary;
    return fingerprint;
}

[[nodiscard]] auto is_divider_line(std::string_view line) noexcept -> bool {
    // CommonMark requires a thematic break to consist ONLY of the marker
    // character (plus optional spaces, which chat content won't have here).
    // The previous x3::lit("---") check matched as soon as it saw the first
    // three dashes and silently discarded whatever followed on the same
    // line -- e.g. "---not actually a divider" lost everything after the
    // dashes. Requiring the whole line to be dashes fixes that data loss.
    if (line.size() < local_constants::minimum_divider_length) {
        return false;
    }
    return std::ranges::all_of(line, [](char character) { return character == '-'; });
}

[[nodiscard]] auto is_ordered_list_line(std::string_view line, std::size_t &prefix_length) noexcept
    -> bool {
    std::size_t digit_count = 0;
    while (digit_count < line.size() &&
           (std::isdigit(static_cast<unsigned char>(line[digit_count])) != 0)) {
        ++digit_count;
    }
    if (digit_count == 0 || line.size() < digit_count + 2) {
        return false;
    }
    if (line[digit_count] != '.' || line[digit_count + 1] != ' ') {
        return false;
    }
    prefix_length = digit_count + 2;
    return true;
}

}  // namespace

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
            token.m_content = std::move(m_buffer);
            token.m_language = std::move(m_language);
            m_tokens.push_back(std::move(token));
            m_buffer.clear();
            m_language.clear();
        }
    }
};

static void parse_markdown_elements(std::string_view line_view, TokenizerState &state) {
    std::size_t ordered_prefix_length = 0;

    if (line_view.starts_with(local_constants::head_4_tag)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_view.substr(local_constants::head_4_tag.size()));
        state.push_buffer(token_type::header_4);
    } else if (line_view.starts_with(local_constants::head_3_tag)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_view.substr(local_constants::head_3_tag.size()));
        state.push_buffer(token_type::header_3);
    } else if (line_view.starts_with(local_constants::head_2_tag)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_view.substr(local_constants::head_2_tag.size()));
        state.push_buffer(token_type::header_2);
    } else if (line_view.starts_with(local_constants::head_1_tag)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_view.substr(local_constants::head_1_tag.size()));
        state.push_buffer(token_type::header_1);
    } else if (is_divider_line(line_view)) {
        state.push_buffer(token_type::paragraph);
        state.push_buffer(token_type::divider);
    } else if (line_view.starts_with(local_constants::list_star_tag) ||
               line_view.starts_with(local_constants::list_dash_tag)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_view.substr(local_constants::list_star_tag.size()));
        state.push_buffer(token_type::list_unordered);
    } else if (is_ordered_list_line(line_view, ordered_prefix_length)) {
        state.push_buffer(token_type::paragraph);
        state.m_buffer = std::string(line_view.substr(ordered_prefix_length));
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
            state.m_buffer += std::string(line_view);
            state.m_buffer += local_constants::spaces_accumulation;
        }
    }
}

static void evaluate_line_tokens(std::string_view line_view, TokenizerState &state) {
    if (line_view.starts_with(local_constants::fence_tag)) {
        if (state.m_in_block) {
            state.push_buffer(token_type::code_block);
            state.m_in_block = false;
        } else {
            state.push_buffer(token_type::paragraph);
            state.m_in_block = true;
            state.m_language = std::string(line_view.substr(local_constants::fence_tag.size()));
        }
        return;
    }

    if (state.m_in_block) {
        state.m_buffer += std::string(line_view);
        state.m_buffer += "\n";
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
    const std::string fingerprint = theme_fingerprint(m_theme);
    const std::size_t cache_key =
        hash_with_seed(text_content, hash_with_seed(fingerprint, fnv_offset_basis));

    if (auto cached = inline_text_cache().get(cache_key)) {
        return std::move(*cached);
    }

    static const boost::regex bold_pattern(R"(\*\*(.*?)\*\*)");
    static const boost::regex code_pattern(R"(`(.*?)`)");

    std::string processed{text_content};
    processed = boost::regex_replace(processed, bold_pattern, "<b>$1</b>");

    std::string inline_code_tag =
        "<font color=\"" + m_theme.m_code_string + R"(" face="monospace">$1</font>)";
    processed = boost::regex_replace(processed, code_pattern, inline_code_tag);

    inline_text_cache().put(cache_key, processed);
    return processed;
}

auto Pipeline::decorate_code_block(std::string_view code, const std::string &lang) const
    -> std::string {
    const std::string fingerprint = theme_fingerprint(m_theme);
    std::size_t cache_key = hash_with_seed(code, fnv_offset_basis);
    cache_key = hash_with_seed(lang, cache_key);
    cache_key = hash_with_seed(fingerprint, cache_key);

    if (auto cached = code_block_cache().get(cache_key)) {
        return std::move(*cached);
    }

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

                auto start_offset = static_cast<std::ptrdiff_t>(current_pos);
                auto start_iter = std::next(processed.cbegin(), start_offset);
                auto final_iter = processed.cend();

                const auto &rule_ref = syntax->m_rules[rule_idx];
                if (boost::regex_search(start_iter, final_iter, match_results,
                                        rule_ref.m_compiled_pattern)) {
                    const std::size_t match_start =
                        current_pos + static_cast<std::size_t>(match_results.position());
                    const std::size_t match_final =
                        match_start + static_cast<std::size_t>(match_results.length());
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

    code_block_cache().put(cache_key, html_output);
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

        // Fixed: the previous version had a stray space inside the quotes
        // (e.g. bgcolor=" #1a0105 "), which wxHtmlWindow's attribute parser
        // does not tolerate for hex colors -- it silently fell back to the
        // default color instead of applying the theme.
        if (row_idx == 0) {
            html_output += R"(<th bgcolor=")" + theme.m_surface_color + R"("><b><font color=")" +
                           theme.m_text_accent + R"(">)" + decorator(trimmed) +
                           R"(</font></b></th>)";
        } else {
            html_output += R"(<td><font color=")" + theme.m_text_primary + R"(">)" +
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
            case token_type::header_4:
                handle_header(token_ref, html_output, local_constants::header_4_size);
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

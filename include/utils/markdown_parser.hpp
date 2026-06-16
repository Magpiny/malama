#pragma once

#include <string>
#include <string_view>
#include <sstream>

namespace malama::utils {

[[nodiscard]] inline auto markdown_to_html(std::string_view markdown_text) -> std::string {
    std::string html_output;
    html_output.reserve(markdown_text.size() * 2);

    std::string_view remaining = markdown_text;
    bool inside_code_block = false;
    bool inside_list = false;

    while (!remaining.empty()) {
        std::size_t newline_pos = remaining.find('\n');
        std::string_view line = (newline_pos == std::string_view::npos) 
                                ? remaining 
                                : remaining.substr(0, newline_pos);

        if (newline_pos == std::string_view::npos) {
            remaining = "";
        } else {
            remaining.remove_prefix(newline_pos + 1);
        }

        // Handle Code Blocks
        if (line.starts_with("```")) {
            if (inside_code_block) {
                html_output += "</code></pre></div>";
                inside_code_block = false;
            } else {
                html_output += "<div style=\"background-color:#1a0105; padding:8px; margin:4px; font-family:monospace;\"><pre><code>";
                inside_code_block = true;
            }
            continue;
        }

        if (inside_code_block) {
            html_output += line;
            html_output += "<br/>";
            continue;
        }

        // Handle Headers
        if (line.starts_with("### ")) {
            html_output += "<h3><font color=\"#c4929a\">" + std::string(line.substr(4)) + "</font></h3>";
            continue;
        } if (line.starts_with("## ")) {
            html_output += "<h2><font color=\"#c4929a\">" + std::string(line.substr(3)) + "</font></h2>";
            continue;
        } if (line.starts_with("# ")) {
            html_output += "<h1><font color=\"#c4929a\">" + std::string(line.substr(2)) + "</font></h1>";
            continue;
        }

        // Handle Unordered Lists
        if (line.starts_with("* ") || line.starts_with("- ")) {
            if (!inside_list) {
                html_output += "<ul>";
                inside_list = true;
            }
            html_output += "<li>" + std::string(line.substr(2)) + "</li>";
            continue;
        } if (inside_list && line.empty()) {
            html_output += "</ul>";
            inside_list = false;
        }

        // Inline formatting replacements (Bold & Inline Code)
        std::string processed_line = std::string(line);
        
        // Basic Markdown bold tracking (**text**)
        std::size_t bold_start = 0;
        while ((bold_start = processed_line.find("**", bold_start)) != std::string::npos) {
            std::size_t bold_end = processed_line.find("**", bold_start + 2);
            if (bold_end == std::string::npos) { break;
}
            
            processed_line.replace(bold_end, 2, "</b>");
            processed_line.replace(bold_start, 2, "<b>");
            bold_start = bold_end + 7; 
        }

        // Basic Inline Code tracking (`code`)
        std::size_t code_start = 0;
        while ((code_start = processed_line.find('`', code_start)) != std::string::npos) {
            std::size_t code_end = processed_line.find('`', code_start + 1);
            if (code_end == std::string::npos) { break;
}
            
            processed_line.replace(code_end, 1, "</code>");
            processed_line.replace(code_start, 1, "<code style=\"background-color:#1a0105;\">");
            code_start = code_end + 39;
        }

        if (processed_line.empty()) {
            html_output += "<br/>";
        } else {
            html_output += "<p>" + processed_line + "</p>";
        }
    }

    if (inside_list) {
        html_output += "</ul>";
    }

    return html_output;
}

} // namespace malama::utils

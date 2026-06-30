// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/markdown/syntax_registry.hpp
// Purpose:     Pluggable, JSON-ready grammar definitions for syntax highlighting
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>

namespace malama::engine::markdown {

// Maps a regex pattern to an intermediate replacement containing control characters
struct SyntaxRule final {
    std::string m_pattern_string;
    std::regex m_compiled_pattern;
    std::string m_replacement_format; 
};

// Represents a complete grammar definition for a language
struct LanguageSyntax final {
    std::string m_name;
    std::vector<SyntaxRule> m_rules;
};

// The centralized grammar manager
class SyntaxRegistry final {
public:
    SyntaxRegistry(); 
    ~SyntaxRegistry() = default;
    
    auto LoadFromJson(const std::string& filepath) -> bool; 
    
    [[nodiscard]] auto GetSyntaxFor(
        const std::string& lang_id
    ) const noexcept -> const LanguageSyntax*;

private:
    void RegisterBuiltinGrammars() noexcept;

    std::unordered_map<std::string, LanguageSyntax> m_language_map;
};

} // namespace malama::engine::markdown

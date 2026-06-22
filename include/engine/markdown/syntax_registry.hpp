// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/markdown/syntax_registry.hpp
// Purpose:     Pluggable, JSON-ready grammer definitions for syntax highlighting
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>

namespace malama::engine::markdown {

// Maps a regex pattern to an intermediate replacement containing control characters
struct SyntaxRule {
    std::regex m_pattern;
    std::string m_replacement; // e.g., "$1\x07$2\x08"
};

// Represents a complete grammar definition for a language
struct LanguageSyntax {
    std::string m_name;
    std::vector<SyntaxRule> m_rules;
};

// The centralized grammar manager
class SyntaxRegistry {
public:
    SyntaxRegistry(); 
    
    // Future-proofing: Pluggable user grammars
    auto LoadFromJson(const std::string& filepath) -> bool; 
    
    [[nodiscard]] auto GetSyntaxFor(const std::string& lang_id) const -> const LanguageSyntax*;

private:
    void RegisterBuiltinGrammars() noexcept;

    std::unordered_map<std::string, LanguageSyntax> m_language_map;
};

} // namespace malama::engine::markdown

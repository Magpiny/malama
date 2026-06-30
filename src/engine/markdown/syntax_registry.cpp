// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/syntax_registry.cpp
// Purpose:     Implements pre-compiled language grammars with non-whitespace tokens
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/markdown/syntax_registry.hpp"

namespace malama::engine::markdown {

SyntaxRegistry::SyntaxRegistry() {
    RegisterBuiltinGrammars();
}

auto SyntaxRegistry::LoadFromJson([[maybe_unused]] const std::string& filepath) -> bool {
    return false;
}

auto SyntaxRegistry::GetSyntaxFor(
    const std::string& lang_id
) const noexcept -> const LanguageSyntax* {
    auto iter = m_language_map.find(lang_id);
    if (iter != m_language_map.end()) {
        return &(iter->second);
    }
    return nullptr;
}

void SyntaxRegistry::RegisterBuiltinGrammars() noexcept {
    // Control Marker Map (Purged \x0A and \x0D whitespace conflicts):
    // \x01 \x02 : Theme String
    // \x03 \x04 : Theme Comment
    // \x05 \x06 : Theme Keyword
    // \x07 \x08 : Entity (Class/Enum/Func) -> Yellow
    // \x0F \x10 : Punctuation (Braces)     -> Orange
    // \x11 \x12 : Headers/Includes         -> Green
    // \x13 \x14 : Methods                  -> Dark Green

    // -------------------------------------------------------------------------
    // 1. C++
    // -------------------------------------------------------------------------
    LanguageSyntax cpp;
    cpp.m_name = "cpp";

    std::string cp1 = R"((\"[^\"]*\"))";
    std::regex cr1(cp1);
    cpp.m_rules.push_back({ cp1, cr1, "\x01$1\x02" });

    std::string cp2 = R"((//.*|/\*[\s\S]*?\*/))";
    std::regex cr2(cp2);
    cpp.m_rules.push_back({ cp2, cr2, "\x03$1\x04" });

    std::string cp3 = R"((#include\s+)(<[^>]+>|\"[^\"]+\"))";
    std::regex cr3(cp3);
    cpp.m_rules.push_back({ cp3, cr3, "\x05$1\x06\x11$2\x12" });

    std::string cp4 = R"(\b([a-zA-Z_]\w*)::([a-zA-Z_]\w*)\s*(?=\())";
    std::regex cr4(cp4);
    cpp.m_rules.push_back({ cp4, cr4, "\x07$1\x08::\x13$2\x14" });

    std::string cp5 = R"(\b([a-zA-Z_]\w*)::([a-zA-Z_]\w*))";
    std::regex cr5(cp5);
    cpp.m_rules.push_back({ cp5, cr5, "\x07$1\x08::$2" });

    std::string cp6 = R"(\b(class|struct|enum)\s+([a-zA-Z_]\w*))";
    std::regex cr6(cp6);
    cpp.m_rules.push_back({ cp6, cr6, "\x05$1\x06 \x07$2\x08" });

    std::string cp7 = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
    std::regex cr7(cp7);
    cpp.m_rules.push_back({ cp7, cr7, "\x07$1\x08" });

    std::string cp8 = R"((#\s*[a-zA-Z]+))";
    std::regex cr8(cp8);
    cpp.m_rules.push_back({ cp8, cr8, "\x05$1\x06" });

    std::string cp9 = R"(\b(auto|const|int|void|std|return|public|private|protected|)";
    cp9 += R"(namespace|using|template|typename|new|delete|if|else|while|for)\b)";
    std::regex cr9(cp9);
    cpp.m_rules.push_back({ cp9, cr9, "\x05$1\x06" });

    std::string cp10 = R"(([\{\}\[\]\(\)]))";
    std::regex cr10(cp10);
    cpp.m_rules.push_back({ cp10, cr10, "\x0F$1\x10" });

    m_language_map["cpp"] = cpp;
    m_language_map["c++"] = cpp;

    // -------------------------------------------------------------------------
    // 2. Python
    // -------------------------------------------------------------------------
    LanguageSyntax python;
    python.m_name = "python";

    std::string pp1 = R"((\"[^\"]*\"|\'[^\']*\'))";
    std::regex pr1(pp1);
    python.m_rules.push_back({ pp1, pr1, "\x01$1\x02" });

    std::string pp2 = R"((#.*))";
    std::regex pr2(pp2);
    python.m_rules.push_back({ pp2, pr2, "\x03$1\x04" });

    std::string pp3 = R"((@[a-zA-Z_]\w*))";
    std::regex pr3(pp3);
    python.m_rules.push_back({ pp3, pr3, "\x07$1\x08" });

    std::string pp4 = R"(\b(class|def)\s+([a-zA-Z_]\w*))";
    std::regex pr4(pp4);
    python.m_rules.push_back({ pp4, pr4, "\x05$1\x06 \x07$2\x08" });

    std::string pp5 = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
    std::regex pr5(pp5);
    python.m_rules.push_back({ pp5, pr5, "\x13$1\x14" });

    std::string pp6 = R"(\b(return|if|else|elif|for|while|import|from|in|is|and|or|not|)";
    pp6 += R"(True|False|None|self|pass|break|continue)\b)";
    std::regex pr6(pp6);
    python.m_rules.push_back({ pp6, pr6, "\x05$1\x06" });

    std::string pp7 = R"(([\{\}\[\]\(\)]))";
    std::regex pr7(pp7);
    python.m_rules.push_back({ pp7, pr7, "\x0F$1\x10" });

    m_language_map["python"] = python;
    m_language_map["py"] = python;
}

} // namespace malama::engine::markdown

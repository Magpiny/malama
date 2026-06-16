// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/syntax_registry.cpp
// Purpose:     Implements builtin language grammars
// /////////////////////////////////////////////////////////////////////////////

#include "engine/markdown/syntax_registry.hpp"

namespace malama::engine::markdown {

SyntaxRegistry::SyntaxRegistry() {
    RegisterBuiltinGrammars();
}

auto SyntaxRegistry::LoadFromJson([[maybe_unused]] const std::string& filepath) -> bool {
    // To be implemented in v0.2.0 for chartra/malama user overrides
    return false;
}

auto SyntaxRegistry::GetSyntaxFor(const std::string& lang_id) const -> const LanguageSyntax* {
    auto iter = m_language_map.find(lang_id);
    if (iter != m_language_map.end()) {
        return &(iter->second);
    }
    return nullptr;
}

void SyntaxRegistry::RegisterBuiltinGrammars() noexcept {
    // Control Marker Map:
    // \x01 \x02 : Theme String
    // \x03 \x04 : Theme Comment
    // \x05 \x06 : Theme Keyword
    // \x07 \x08 : Entity (Class/Enum/Func) -> Yellow
    // \x09 \x0A : Punctuation (Braces) -> Orange
    // \x0B \x0C : Headers/Includes -> Green
    // \x0D \x0E : Methods -> Dark Green

    // -------------------------------------------------------------------------
    // 1. C++
    // -------------------------------------------------------------------------
    LanguageSyntax cpp;
    cpp.m_name = "cpp";
    cpp.m_rules = {
        {.m_pattern=std::regex(R"((\"[^\"]*\"))"), .m_replacement="\x01$1\x02"}, // Strings
        {.m_pattern=std::regex(R"((//.*|/\*[\s\S]*?\*/))"), .m_replacement="\x03$1\x04"}, // Comments
        {.m_pattern=std::regex(R"((#include\s+)(<[^>]+>|\"[^\"]+\"))"), .m_replacement="\x05$1\x06\x0B$2\x0C"}, // Includes (Green)
        {.m_pattern=std::regex(R"(\b([a-zA-Z_]\w*)::([a-zA-Z_]\w*)\s*(?=\())"), .m_replacement="\x07$1\x08::\x0D$2\x0E"}, // namespace::class::method (Class Yellow, Method Dark Green)
        {.m_pattern=std::regex(R"(\b([a-zA-Z_]\w*)::([a-zA-Z_]\w*))"), .m_replacement="\x07$1\x08::$2"}, // General Scope (Class Yellow)
        {.m_pattern=std::regex(R"(\b(class|struct|enum)\s+([a-zA-Z_]\w*))"), .m_replacement="\x05$1\x06 \x07$2\x08"}, // Class/Struct/Enum Decl (Yellow)
        {.m_pattern=std::regex(R"(\b([a-zA-Z_]\w*)\s*(?=\())"), .m_replacement="\x07$1\x08"}, // Function Names (Yellow)
        {.m_pattern=std::regex(R"((#\s*[a-zA-Z]+))"), .m_replacement="\x05$1\x06"}, // Preprocessor
        {.m_pattern=std::regex(R"(\b(auto|const|int|void|std|return|public|private|protected|namespace|using|template|typename|new|delete|if|else|while|for)\b)"), .m_replacement="\x05$1\x06"}, // Keywords
        {.m_pattern=std::regex(R"(([\{\}\[\]\(\)]))"), .m_replacement="\x09$1\x0A"} // Punctuation (Orange)
    };
    m_language_map["cpp"] = cpp;
    m_language_map["c++"] = cpp;

    // -------------------------------------------------------------------------
    // 2. Python
    // -------------------------------------------------------------------------
    LanguageSyntax python;
    python.m_name = "python";
    python.m_rules = {
        {.m_pattern=std::regex(R"((\"[^\"]*\"|\'[^\']*\'))"), .m_replacement="\x01$1\x02"},
        {.m_pattern=std::regex(R"((#.*))"), .m_replacement="\x03$1\x04"},
        {.m_pattern=std::regex(R"((@[a-zA-Z_]\w*))"), .m_replacement="\x07$1\x08"}, // Decorators (Yellow)
        {.m_pattern=std::regex(R"(\b(class|def)\s+([a-zA-Z_]\w*))"), .m_replacement="\x05$1\x06 \x07$2\x08"},
        {.m_pattern=std::regex(R"(\b([a-zA-Z_]\w*)\s*(?=\())"), .m_replacement="\x0D$1\x0E"}, // Methods (Dark Green)
        {.m_pattern=std::regex(R"(\b(return|if|else|elif|for|while|import|from|in|is|and|or|not|True|False|None|self|pass|break|continue)\b)"), .m_replacement="\x05$1\x06"},
        {.m_pattern=std::regex(R"(([\{\}\[\]\(\)]))"), .m_replacement="\x09$1\x0A"}
    };
    m_language_map["python"] = python;
    m_language_map["py"] = python;

    // -------------------------------------------------------------------------
    // 3. Rust
    // -------------------------------------------------------------------------
    LanguageSyntax rust;
    rust.m_name = "rust";
    rust.m_rules = {
        {.m_pattern=std::regex(R"((\"[^\"]*\"))"), .m_replacement="\x01$1\x02"},
        {.m_pattern=std::regex(R"((//.*))"), .m_replacement="\x03$1\x04"},
        {.m_pattern=std::regex(R"(\b([a-zA-Z_]\w*!)\s*(?=[\[\(]))"), .m_replacement="\x0D$1\x0E"}, // Macros (Dark Green)
        {.m_pattern=std::regex(R"(\b(struct|enum|fn|trait|impl)\s+([a-zA-Z_]\w*))"), .m_replacement="\x05$1\x06 \x07$2\x08"},
        {.m_pattern=std::regex(R"(\b([a-zA-Z_]\w*)\s*(?=\())"), .m_replacement="\x07$1\x08"},
        {.m_pattern=std::regex(R"(\b(let|mut|pub|return|if|else|match|use|mod|crate|for|in|as|ref|Self|String)\b)"), .m_replacement="\x05$1\x06"},
        {.m_pattern=std::regex(R"((\'[a-zA-Z_]\w*))"), .m_replacement="\x09$1\x0A"}, // Lifetimes (Orange)
        {.m_pattern=std::regex(R"(([\{\}\[\]\(\)]))"), .m_replacement="\x09$1\x0A"}
    };
    m_language_map["rust"] = rust;
    m_language_map["rs"] = rust;

    // -------------------------------------------------------------------------
    // 4. JavaScript
    // -------------------------------------------------------------------------
    LanguageSyntax javascript;
    javascript.m_name = "javascript";
    javascript.m_rules = {
        {.m_pattern=std::regex(R"((\".*?\"|\'.*?\'|\`.*?\`))"), .m_replacement="\x01$1\x02"},
        {.m_pattern=std::regex(R"((//.*|/\*[\s\S]*?\*/))"), .m_replacement="\x03$1\x04"},
        {.m_pattern=std::regex(R"(\b(class|function)\s+([a-zA-Z_]\w*))"), .m_replacement="\x05$1\x06 \x07$2\x08"},
        {.m_pattern=std::regex(R"(\b([a-zA-Z_]\w*)\s*(?=\())"), .m_replacement="\x0D$1\x0E"},
        {.m_pattern=std::regex(R"(\b(const|let|var|return|if|else|for|while|import|export|from|extends|new|this|await|async)\b)"), .m_replacement="\x05$1\x06"},
        {.m_pattern=std::regex(R"(([\{\}\[\]\(\)]))"), .m_replacement="\x09$1\x0A"}
    };
    m_language_map["javascript"] = javascript;
    m_language_map["js"] = javascript;

    // -------------------------------------------------------------------------
    // 5. CSS
    // -------------------------------------------------------------------------
    LanguageSyntax css;
    css.m_name = "css";
    css.m_rules = {
        {.m_pattern=std::regex(R"((/\*[\s\S]*?\*/))"), .m_replacement="\x03$1\x04"},
        {.m_pattern=std::regex(R"(([\.#]?[a-zA-Z0-9_-]+)\s*(?=\{))"), .m_replacement="\x07$1\x08"}, // Selectors (Yellow)
        {.m_pattern=std::regex(R"(([a-zA-Z-]+)\s*(?=:))"), .m_replacement="\x0B$1\x0C"}, // Properties (Green)
        {.m_pattern=std::regex(R"((:\s*)([^;]+))"), .m_replacement="$1\x01$2\x02"}, // Values (String color)
        {.m_pattern=std::regex(R"(([\{\}]))"), .m_replacement="\x09$1\x0A"}
    };
    m_language_map["css"] = css;
}

} // namespace malama::engine::markdown

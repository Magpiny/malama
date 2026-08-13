// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/syntax_registry.cpp
// Purpose:     Implements language grammars with Boost.Regex execution pipelines
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/markdown/syntax_registry.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace malama::engine::markdown {

SyntaxRegistry::SyntaxRegistry() {
    RegisterBuiltinGrammars();
}

auto SyntaxRegistry::LoadFromJson([[maybe_unused]] const std::string &filepath) -> bool {
    return false;
}

auto SyntaxRegistry::GetSyntaxFor(const std::string &lang_id) const noexcept
    -> const LanguageSyntax * {
    auto iter = m_language_map.find(lang_id);
    if (iter != m_language_map.end()) {
        return &(iter->second);
    }
    return nullptr;
}

void SyntaxRegistry::RegisterBuiltinGrammars() noexcept {
    static const std::unordered_map<std::string, LanguageSyntax> builtin_cache = []() {
        std::unordered_map<std::string, LanguageSyntax> cache;

        // ---------------------------------------------------------------------
        // 1. C++ Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_cpp;
        lang_cpp.m_name = "cpp";

        std::string pat_cpp_string = R"((\"[^\"]*\"))";
        lang_cpp.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_cpp_string,
                                              .m_compiled_pattern = boost::regex(pat_cpp_string),
                                              .m_replacement_format = "\x01$1\x02"});

        std::string pat_cpp_comment = R"((//[^\n]*|/\*[\s\S]*?\*/))";
        lang_cpp.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_cpp_comment,
                                              .m_compiled_pattern = boost::regex(pat_cpp_comment),
                                              .m_replacement_format = "\x03$1\x04"});

        std::string pat_cpp_include = R"((#include\s+)(<[^>]+>|\"[^\"]+\"))";
        lang_cpp.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_cpp_include,
                                              .m_compiled_pattern = boost::regex(pat_cpp_include),
                                              .m_replacement_format = "\x05$1\x06\x11$2\x12"});

        std::string pat_cpp_method1 = R"(\b([a-zA-Z_]\w*)::([a-zA-Z_]\w*)\s*(?=\())";
        lang_cpp.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_cpp_method1,
                                              .m_compiled_pattern = boost::regex(pat_cpp_method1),
                                              .m_replacement_format = "\x07$1\x08::\x13$2\x14"});

        std::string pat_cpp_method2 = R"(\b([a-zA-Z_]\w*)::([a-zA-Z_]\w*))";
        lang_cpp.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_cpp_method2,
                                              .m_compiled_pattern = boost::regex(pat_cpp_method2),
                                              .m_replacement_format = "\x07$1\x08::$2"});

        std::string pat_cpp_class = R"(\b(class|struct|enum)\s+([a-zA-Z_]\w*))";
        lang_cpp.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_cpp_class,
                                              .m_compiled_pattern = boost::regex(pat_cpp_class),
                                              .m_replacement_format = "\x05$1\x06 \x07$2\x08"});

        std::string pat_cpp_func = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_cpp.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_cpp_func,
                                              .m_compiled_pattern = boost::regex(pat_cpp_func),
                                              .m_replacement_format = "\x07$1\x08"});

        std::string pat_cpp_preproc = R"((#\s*[a-zA-Z]+))";
        lang_cpp.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_cpp_preproc,
                                              .m_compiled_pattern = boost::regex(pat_cpp_preproc),
                                              .m_replacement_format = "\x05$1\x06"});

        std::string pat_cpp_keyword = R"(\b(auto|const|constexpr|std|int|void|)";
        pat_cpp_keyword += R"(return|public|private|string|catch|decltype|)";
        pat_cpp_keyword += R"(protected|namespace|using|template|typename|)";
        pat_cpp_keyword += R"(new|delete|if|&&|&=|!|!=|&|asm|typeid|throw|)";
        pat_cpp_keyword += R"(try|function|goto|do|explicit|export|import|)";
        pat_cpp_keyword += R"(noexcept|co_await|co_yield|co_return|inline|)";
        pat_cpp_keyword += R"(operator|sizeof|static|else|while|for|virtual|)";
        pat_cpp_keyword += R"(enum|char|bool|switch|this|final|override|long|)";
        pat_cpp_keyword += R"(short|thread|jthread|break)\b)";
        lang_cpp.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_cpp_keyword,
                                              .m_compiled_pattern = boost::regex(pat_cpp_keyword),
                                              .m_replacement_format = "\x05$1\x06"});

        std::string pat_cpp_punct = R"(([\{\}\[\]\(\)]))";
        lang_cpp.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_cpp_punct,
                                              .m_compiled_pattern = boost::regex(pat_cpp_punct),
                                              .m_replacement_format = "\x0F$1\x10"});

        cache["cpp"] = lang_cpp;
        cache["c++"] = lang_cpp;
        cache["c"] = lang_cpp;

        // ---------------------------------------------------------------------
        // 2. Python Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_python;
        lang_python.m_name = "python";

        std::string pat_py_multiline = R"raw(("""[\s\S]*?"""|'''[\s\S]*?'''))raw";
        lang_python.m_rules.push_back(
            SyntaxRule{.m_pattern_string = pat_py_multiline,
                       .m_compiled_pattern = boost::regex(pat_py_multiline),
                       .m_replacement_format = "\x01$1\x02"});

        std::string pat_py_string = R"((\"[^\"]*\"|\'[^\']*\'))";
        lang_python.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_py_string,
                                                 .m_compiled_pattern = boost::regex(pat_py_string),
                                                 .m_replacement_format = "\x01$1\x02"});

        std::string pat_py_comment = R"((#[^\n]*))";
        lang_python.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_py_comment,
                                                 .m_compiled_pattern = boost::regex(pat_py_comment),
                                                 .m_replacement_format = "\x03$1\x04"});

        std::string pat_py_decor = R"((@[a-zA-Z_]\w*))";
        lang_python.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_py_decor,
                                                 .m_compiled_pattern = boost::regex(pat_py_decor),
                                                 .m_replacement_format = "\x07$1\x08"});

        std::string pat_py_class = R"(\b(class|def)\s+([a-zA-Z_]\w*))";
        lang_python.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_py_class,
                                                 .m_compiled_pattern = boost::regex(pat_py_class),
                                                 .m_replacement_format = "\x05$1\x06 \x07$2\x08"});

        std::string pat_py_func = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_python.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_py_func,
                                                 .m_compiled_pattern = boost::regex(pat_py_func),
                                                 .m_replacement_format = "\x13$1\x14"});

        std::string pat_py_keyword = R"(\b(return|if|else|elif|for|while|import|yield|from|)";
        pat_py_keyword += R"(in|is|kwargs|args|def|not|class|and|or|list|dict|tuple|)";
        pat_py_keyword += R"(set|str|int|float|bool|not|True|False|None|self|pass|)";
        pat_py_keyword += R"(break|continue)\b)";
        lang_python.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_py_keyword,
                                                 .m_compiled_pattern = boost::regex(pat_py_keyword),
                                                 .m_replacement_format = "\x05$1\x06"});

        std::string pat_py_punct = R"(([\{\}\[\]\(\)]))";
        lang_python.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_py_punct,
                                                 .m_compiled_pattern = boost::regex(pat_py_punct),
                                                 .m_replacement_format = "\x0F$1\x10"});

        cache["python"] = lang_python;
        cache["py"] = lang_python;

        // ---------------------------------------------------------------------
        // 3. CSS Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_css;
        lang_css.m_name = "css";

        std::string pat_css_string = R"(("[^"]*"|'[^']*'))";
        lang_css.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_css_string,
                                              .m_compiled_pattern = boost::regex(pat_css_string),
                                              .m_replacement_format = "\x01$1\x02"});

        std::string pat_css_comment = R"((\/\*[\s\S]*?\*\/))";
        lang_css.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_css_comment,
                                              .m_compiled_pattern = boost::regex(pat_css_comment),
                                              .m_replacement_format = "\x03$1\x04"});

        std::string pat_css_selector = R"(([\.#][a-zA-Z_][-a-zA-Z0-9_]*))";
        lang_css.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_css_selector,
                                              .m_compiled_pattern = boost::regex(pat_css_selector),
                                              .m_replacement_format = "\x07$1\x08"});

        std::string pat_css_property = R"(\b([a-zA-Z_-]+)\s*(?=:))";
        lang_css.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_css_property,
                                              .m_compiled_pattern = boost::regex(pat_css_property),
                                              .m_replacement_format = "\x05$1\x06"});

        std::string pat_css_punct = R"(([\{\}\(\)]))";
        lang_css.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_css_punct,
                                              .m_compiled_pattern = boost::regex(pat_css_punct),
                                              .m_replacement_format = "\x0F$1\x10"});

        cache["css"] = lang_css;

        // ---------------------------------------------------------------------
        // 4. HTML Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_html;
        lang_html.m_name = "html";

        // FIX: Properly initialize pat_html_comment pattern string
        std::string pat_html_comment = R"(())";
        lang_html.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_html_comment,
                                               .m_compiled_pattern = boost::regex(pat_html_comment),
                                               .m_replacement_format = "\x03$1\x04"});

        std::string pat_html_tag = R"((<\/?[a-zA-Z0-9:-]+))";
        lang_html.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_html_tag,
                                               .m_compiled_pattern = boost::regex(pat_html_tag),
                                               .m_replacement_format = "\x05$1\x06"});

        std::string pat_html_attr = R"(\b([a-zA-Z_-]+)=)";
        lang_html.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_html_attr,
                                               .m_compiled_pattern = boost::regex(pat_html_attr),
                                               .m_replacement_format = "\x07$1\x08="});

        std::string pat_html_string = R"(("[^"]*"|'[^']*'))";
        lang_html.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_html_string,
                                               .m_compiled_pattern = boost::regex(pat_html_string),
                                               .m_replacement_format = "\x01$1\x02"});

        cache["html"] = lang_html;

        // ---------------------------------------------------------------------
        // 5. JavaScript / TypeScript Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_js;
        lang_js.m_name = "javascript";

        std::string pat_js_string = R"(("[^"]*"|'[^']*'|`[^`]*`))";
        lang_js.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_js_string,
                                             .m_compiled_pattern = boost::regex(pat_js_string),
                                             .m_replacement_format = "\x01$1\x02"});

        std::string pat_js_comment = R"((//[^\n]*|/\*[\s\S]*?\*/))";
        lang_js.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_js_comment,
                                             .m_compiled_pattern = boost::regex(pat_js_comment),
                                             .m_replacement_format = "\x03$1\x04"});

        std::string pat_js_keyword = R"(\b(const|new|this|super|try|let|var|)";
        pat_js_keyword += R"(function|class|extends|return|if|else|for|while|)";
        pat_js_keyword += R"(import|export|from|async|interface|type|catch|)";
        pat_js_keyword += R"(finally|await|true|false|null|as|is|any|unknown|)";
        pat_js_keyword += R"(never|void|keyof|typeof|satisfies|yield|public|)";
        pat_js_keyword += R"(private|protected|readonly|static|implements|)";
        pat_js_keyword += R"(undefined|console|log|switch|case|default|enum|)";
        pat_js_keyword += R"(namespace|module|break|continue|do)\b)";
        lang_js.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_js_keyword,
                                             .m_compiled_pattern = boost::regex(pat_js_keyword),
                                             .m_replacement_format = "\x05$1\x06"});

        std::string pat_js_method = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_js.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_js_method,
                                             .m_compiled_pattern = boost::regex(pat_js_method),
                                             .m_replacement_format = "\x13$1\x14"});

        std::string pat_js_punct = R"(([\{\}\[\]\(\)]))";
        lang_js.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_js_punct,
                                             .m_compiled_pattern = boost::regex(pat_js_punct),
                                             .m_replacement_format = "\x0F$1\x10"});

        cache["javascript"] = lang_js;
        cache["typescript"] = lang_js;
        cache["js"] = lang_js;
        cache["ts"] = lang_js;

        // ---------------------------------------------------------------------
        // 6. Java Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_java;
        lang_java.m_name = "java";

        std::string pat_java_string = R"(("[^"]*"|'[^']*'))";
        lang_java.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_java_string,
                                               .m_compiled_pattern = boost::regex(pat_java_string),
                                               .m_replacement_format = "\x01$1\x02"});

        std::string pat_java_comment = R"((//[^\n]*|/\*[\s\S]*?\*/))";
        lang_java.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_java_comment,
                                               .m_compiled_pattern = boost::regex(pat_java_comment),
                                               .m_replacement_format = "\x03$1\x04"});

        std::string pat_java_decor = R"((@[a-zA-Z_]\w*))";
        lang_java.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_java_decor,
                                               .m_compiled_pattern = boost::regex(pat_java_decor),
                                               .m_replacement_format = "\x07$1\x08"});

        std::string pat_java_keyword = R"(\b(public|private|protected|class|interface|enum|)";
        pat_java_keyword += R"(extends|implements|static|final|void|int|double|boolean|return|)";
        pat_java_keyword += R"(if|else|for|while|new|this|package|import)\b)";
        lang_java.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_java_keyword,
                                               .m_compiled_pattern = boost::regex(pat_java_keyword),
                                               .m_replacement_format = "\x05$1\x06"});

        std::string pat_java_method = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_java.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_java_method,
                                               .m_compiled_pattern = boost::regex(pat_java_method),
                                               .m_replacement_format = "\x13$1\x14"});

        std::string pat_java_punct = R"(([\{\}\[\]\(\)]))";
        lang_java.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_java_punct,
                                               .m_compiled_pattern = boost::regex(pat_java_punct),
                                               .m_replacement_format = "\x0F$1\x10"});

        cache["java"] = lang_java;

        // ---------------------------------------------------------------------
        // 7. PHP Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_php;
        lang_php.m_name = "php";

        std::string pat_php_string = R"(("[^"]*"|'[^']*'))";
        lang_php.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_php_string,
                                              .m_compiled_pattern = boost::regex(pat_php_string),
                                              .m_replacement_format = "\x01$1\x02"});

        std::string pat_php_comment = R"((//[^\n]*|#[^\n]*|/\*[\s\S]*?\*/))";
        lang_php.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_php_comment,
                                              .m_compiled_pattern = boost::regex(pat_php_comment),
                                              .m_replacement_format = "\x03$1\x04"});

        std::string pat_php_var = R"((\$[a-zA-Z_]\w*))";
        lang_php.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_php_var,
                                              .m_compiled_pattern = boost::regex(pat_php_var),
                                              .m_replacement_format = "\x07$1\x08"});

        std::string pat_php_keyword = R"(\b(function|class|public|private|protected|return|if|)";
        pat_php_keyword += R"(else|elseif|for|foreach|while|echo|namespace|use|new|as)\b)";
        lang_php.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_php_keyword,
                                              .m_compiled_pattern = boost::regex(pat_php_keyword),
                                              .m_replacement_format = "\x05$1\x06"});

        std::string pat_php_method = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_php.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_php_method,
                                              .m_compiled_pattern = boost::regex(pat_php_method),
                                              .m_replacement_format = "\x13$1\x14"});

        std::string pat_php_punct = R"(([\{\}\[\]\(\)]))";
        lang_php.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_php_punct,
                                              .m_compiled_pattern = boost::regex(pat_php_punct),
                                              .m_replacement_format = "\x0F$1\x10"});

        cache["php"] = lang_php;

        // ---------------------------------------------------------------------
        // 8. Rust Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_rust;
        lang_rust.m_name = "rust";

        std::string pat_rust_string = R"(("[^"]*"))";
        lang_rust.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_rust_string,
                                               .m_compiled_pattern = boost::regex(pat_rust_string),
                                               .m_replacement_format = "\x01$1\x02"});

        std::string pat_rust_comment = R"((//[^\n]*|/\*[\s\S]*?\*/))";
        lang_rust.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_rust_comment,
                                               .m_compiled_pattern = boost::regex(pat_rust_comment),
                                               .m_replacement_format = "\x03$1\x04"});

        std::string pat_rust_keyword = R"(\b(fn|let|mut|const|static|ref|match|if|else|for|)";
        pat_rust_keyword += R"(while|loop|return|type|continue|break|extern|where|dyn|)";
        pat_rust_keyword += R"(Box|macro|try|struct|enum|impl|trait|pub|use|mod|move|)";
        pat_rust_keyword += R"(async|await|true|false)\b)";
        lang_rust.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_rust_keyword,
                                               .m_compiled_pattern = boost::regex(pat_rust_keyword),
                                               .m_replacement_format = "\x05$1\x06"});

        std::string pat_rust_macro = R"(\b([a-zA-Z_]\w*!))";
        lang_rust.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_rust_macro,
                                               .m_compiled_pattern = boost::regex(pat_rust_macro),
                                               .m_replacement_format = "\x07$1\x08"});

        std::string pat_rust_method = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_rust.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_rust_method,
                                               .m_compiled_pattern = boost::regex(pat_rust_method),
                                               .m_replacement_format = "\x13$1\x14"});

        std::string pat_rust_punct = R"(([\{\}\[\]\(\)]))";
        lang_rust.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_rust_punct,
                                               .m_compiled_pattern = boost::regex(pat_rust_punct),
                                               .m_replacement_format = "\x0F$1\x10"});

        cache["rust"] = lang_rust;
        cache["rs"] = lang_rust;

        // ---------------------------------------------------------------------
        // 9. Kotlin Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_kotlin;
        lang_kotlin.m_name = "kotlin";

        std::string pat_kot_string = R"(("[^"]*"|'[^']*'))";
        lang_kotlin.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_kot_string,
                                                 .m_compiled_pattern = boost::regex(pat_kot_string),
                                                 .m_replacement_format = "\x01$1\x02"});

        std::string pat_kot_comment = R"((//[^\n]*|/\*[\s\S]*?\*/))";
        lang_kotlin.m_rules.push_back(
            SyntaxRule{.m_pattern_string = pat_kot_comment,
                       .m_compiled_pattern = boost::regex(pat_kot_comment),
                       .m_replacement_format = "\x03$1\x04"});

        std::string pat_kot_keyword = R"(\b(fun|val|var|class|interface|object|return|if|)";
        pat_kot_keyword += R"(else|for|while|when|import|package|public|private|protected|)";
        pat_kot_keyword += R"(internal|this|super|null|true|false)\b)";
        lang_kotlin.m_rules.push_back(
            SyntaxRule{.m_pattern_string = pat_kot_keyword,
                       .m_compiled_pattern = boost::regex(pat_kot_keyword),
                       .m_replacement_format = "\x05$1\x06"});

        std::string pat_kot_method = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_kotlin.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_kot_method,
                                                 .m_compiled_pattern = boost::regex(pat_kot_method),
                                                 .m_replacement_format = "\x13$1\x14"});

        std::string pat_kot_punct = R"(([\{\}\[\]\(\)]))";
        lang_kotlin.m_rules.push_back(SyntaxRule{.m_pattern_string = pat_kot_punct,
                                                 .m_compiled_pattern = boost::regex(pat_kot_punct),
                                                 .m_replacement_format = "\x0F$1\x10"});

        cache["kotlin"] = lang_kotlin;
        cache["kt"] = lang_kotlin;

        return cache;
    }();

    m_language_map = builtin_cache;
}

}  // namespace malama::engine::markdown

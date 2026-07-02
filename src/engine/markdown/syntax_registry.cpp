// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/markdown/syntax_registry.cpp
// Purpose:     Implements language grammars with Boost.Regex execution pipelines
// Author:      Wanjare S. <samuewanjare@protonmail.com>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/markdown/syntax_registry.hpp"
#include <boost/regex.hpp>

namespace malama::engine::markdown {

SyntaxRegistry::SyntaxRegistry() {
    RegisterBuiltinGrammars();
}

auto SyntaxRegistry::LoadFromJson(
    [[maybe_unused]] const std::string& filepath
) -> bool {
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
    // Thread-safe singleton local cache to ensure regexes are only compiled ONCE
    static const std::unordered_map<std::string, LanguageSyntax> builtin_cache = []() {
        std::unordered_map<std::string, LanguageSyntax> cache;

        // ---------------------------------------------------------------------
        // 1. C++ Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_cpp;
        lang_cpp.m_name = "cpp";

        std::string pat_cpp_string = R"((\"[^\"]*\"))";
        lang_cpp.m_rules.push_back({
            pat_cpp_string, boost::regex(pat_cpp_string), "\x01$1\x02"
        });

        std::string pat_cpp_comment = R"((//[^\n]*|/\*[\s\S]*?\*/))";
        lang_cpp.m_rules.push_back({
            pat_cpp_comment, boost::regex(pat_cpp_comment), "\x03$1\x04"
        });

        std::string pat_cpp_include = R"((#include\s+)(<[^>]+>|\"[^\"]+\"))";
        lang_cpp.m_rules.push_back({
            pat_cpp_include, boost::regex(pat_cpp_include), "\x05$1\x06\x11$2\x12"
        });

        std::string pat_cpp_method1 = R"(\b([a-zA-Z_]\w*)::([a-zA-Z_]\w*)\s*(?=\())";
        lang_cpp.m_rules.push_back({
            pat_cpp_method1, boost::regex(pat_cpp_method1), "\x07$1\x08::\x13$2\x14"
        });

        std::string pat_cpp_method2 = R"(\b([a-zA-Z_]\w*)::([a-zA-Z_]\w*))";
        lang_cpp.m_rules.push_back({
            pat_cpp_method2, boost::regex(pat_cpp_method2), "\x07$1\x08::$2"
        });

        std::string pat_cpp_class = R"(\b(class|struct|enum)\s+([a-zA-Z_]\w*))";
        lang_cpp.m_rules.push_back({
            pat_cpp_class, boost::regex(pat_cpp_class), "\x05$1\x06 \x07$2\x08"
        });

        std::string pat_cpp_func = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_cpp.m_rules.push_back({
            pat_cpp_func, boost::regex(pat_cpp_func), "\x07$1\x08"
        });

        std::string pat_cpp_preproc = R"((#\s*[a-zA-Z]+))";
        lang_cpp.m_rules.push_back({
            pat_cpp_preproc, boost::regex(pat_cpp_preproc), "\x05$1\x06"
        });

        std::string pat_cpp_keyword = R"(\b(auto|const|int|void|std|return|public|private|)";
        pat_cpp_keyword += R"(protected|namespace|using|template|typename|new|delete|if|)";
        pat_cpp_keyword += R"(else|while|for)\b)";
        lang_cpp.m_rules.push_back({
            pat_cpp_keyword, boost::regex(pat_cpp_keyword), "\x05$1\x06"
        });

        std::string pat_cpp_punct = R"(([\{\}\[\]\(\)]))";
        lang_cpp.m_rules.push_back({
            pat_cpp_punct, boost::regex(pat_cpp_punct), "\x0F$1\x10"
        });

        cache["cpp"] = lang_cpp;
        cache["c++"] = lang_cpp;

        // ---------------------------------------------------------------------
        // 2. Python Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_python;
        lang_python.m_name = "python";

        // Prioritized triple-quote multi-line processing engine matching pass
        std::string pat_py_multiline = R"raw(砂("""[\s\S]*?"""|'''[\s\S]*?'''))raw";
        lang_python.m_rules.push_back({
            pat_py_multiline, boost::regex(pat_py_multiline), "\x01$1\x02"
        });

        std::string pat_py_string = R"((\"[^\"]*\"|\'[^\']*\'))";
        lang_python.m_rules.push_back({
            pat_py_string, boost::regex(pat_py_string), "\x01$1\x02"
        });

        std::string pat_py_comment = R"((#[^\n]*))";
        lang_python.m_rules.push_back({
            pat_py_comment, boost::regex(pat_py_comment), "\x03$1\x04"
        });

        std::string pat_py_decor = R"((@[a-zA-Z_]\w*))";
        lang_python.m_rules.push_back({
            pat_py_decor, boost::regex(pat_py_decor), "\x07$1\x08"
        });

        std::string pat_py_class = R"(\b(class|def)\s+([a-zA-Z_]\w*))";
        lang_python.m_rules.push_back({
            pat_py_class, boost::regex(pat_py_class), "\x05$1\x06 \x07$2\x08"
        });

        std::string pat_py_func = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_python.m_rules.push_back({
            pat_py_func, boost::regex(pat_py_func), "\x13$1\x14"
        });

        std::string pat_py_keyword = R"(\b(return|if|else|elif|for|while|import|from|in|is|)";
        pat_py_keyword += R"(and|or|not|True|False|None|self|pass|break|continue)\b)";
        lang_python.m_rules.push_back({
            pat_py_keyword, boost::regex(pat_py_keyword), "\x05$1\x06"
        });

        std::string pat_py_punct = R"(([\{\}\[\]\(\)]))";
        lang_python.m_rules.push_back({
            pat_py_punct, boost::regex(pat_py_punct), "\x0F$1\x10"
        });

        cache["python"] = lang_python;
        cache["py"] = lang_python;

        // ---------------------------------------------------------------------
        // 3. CSS Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_css;
        lang_css.m_name = "css";

        std::string pat_css_string = R"(("[^"]*"|'[^']*'))";
        lang_css.m_rules.push_back({
            pat_css_string, boost::regex(pat_css_string), "\x01$1\x02"
        });

        std::string pat_css_comment = R"((\/\*[\s\S]*?\*\/))";
        lang_css.m_rules.push_back({
            pat_css_comment, boost::regex(pat_css_comment), "\x03$1\x04"
        });

        std::string pat_css_selector = R"(([\.#][a-zA-Z_][-a-zA-Z0-9_]*))";
        lang_css.m_rules.push_back({
            pat_css_selector, boost::regex(pat_css_selector), "\x07$1\x08"
        });

        std::string pat_css_property = R"(\b([a-zA-Z_-]+)\s*(?=:))";
        lang_css.m_rules.push_back({
            pat_css_property, boost::regex(pat_css_property), "\x05$1\x06"
        });

        std::string pat_css_punct = R"(([\{\}\(\)]))";
        lang_css.m_rules.push_back({
            pat_css_punct, boost::regex(pat_css_punct), "\x0F$1\x10"
        });

        cache["css"] = lang_css;

        // ---------------------------------------------------------------------
        // 4. HTML Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_html;
        lang_html.m_name = "html";

        std::string pat_html_comment = R"(())";
        lang_html.m_rules.push_back({
            pat_html_comment, boost::regex(pat_html_comment), "\x03$1\x04"
        });

        std::string pat_html_tag = R"((<\/?[a-zA-Z0-9:-]+))";
        lang_html.m_rules.push_back({
            pat_html_tag, boost::regex(pat_html_tag), "\x05$1\x06"
        });

        std::string pat_html_attr = R"(\b([a-zA-Z_-]+)=)";
        lang_html.m_rules.push_back({
            pat_html_attr, boost::regex(pat_html_attr), "\x07$1\x08="
        });

        std::string pat_html_string = R"(("[^"]*"|'[^']*'))";
        lang_html.m_rules.push_back({
            pat_html_string, boost::regex(pat_html_string), "\x01$1\x02"
        });

        cache["html"] = lang_html;

        // ---------------------------------------------------------------------
        // 5. JavaScript / TypeScript Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_js;
        lang_js.m_name = "javascript";

        std::string pat_js_string = R"(("[^"]*"|'[^']*'|`[^`]*`))";
        lang_js.m_rules.push_back({
            pat_js_string, boost::regex(pat_js_string), "\x01$1\x02"
        });

        std::string pat_js_comment = R"((//[^\n]*|/\*[\s\S]*?\*/))";
        lang_js.m_rules.push_back({
            pat_js_comment, boost::regex(pat_js_comment), "\x03$1\x04"
        });

        std::string pat_js_keyword = R"(\b(const|let|var|function|class|extends|return|if|)";
        pat_js_keyword += R"(else|for|while|import|export|from|async|await|true|false|null|)";
        pat_js_keyword += R"(undefined|this|new)\b)";
        lang_js.m_rules.push_back({
            pat_js_keyword, boost::regex(pat_js_keyword), "\x05$1\x06"
        });

        std::string pat_js_method = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_js.m_rules.push_back({
            pat_js_method, boost::regex(pat_js_method), "\x13$1\x14"
        });

        std::string pat_js_punct = R"(([\{\}\[\]\(\)]))";
        lang_js.m_rules.push_back({
            pat_js_punct, boost::regex(pat_js_punct), "\x0F$1\x10"
        });

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
        lang_java.m_rules.push_back({
            pat_java_string, boost::regex(pat_java_string), "\x01$1\x02"
        });

        std::string pat_java_comment = R"((//[^\n]*|/\*[\s\S]*?\*/))";
        lang_java.m_rules.push_back({
            pat_java_comment, boost::regex(pat_java_comment), "\x03$1\x04"
        });

        std::string pat_java_decor = R"((@[a-zA-Z_]\w*))";
        lang_java.m_rules.push_back({
            pat_java_decor, boost::regex(pat_java_decor), "\x07$1\x08"
        });

        std::string pat_java_keyword = R"(\b(public|private|protected|class|interface|enum|)";
        pat_java_keyword += R"(extends|implements|static|final|void|int|double|boolean|return|)";
        pat_java_keyword += R"(if|else|for|while|new|this|package|import)\b)";
        lang_java.m_rules.push_back({
            pat_java_keyword, boost::regex(pat_java_keyword), "\x05$1\x06"
        });

        std::string pat_java_method = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_java.m_rules.push_back({
            pat_java_method, boost::regex(pat_java_method), "\x13$1\x14"
        });

        std::string pat_java_punct = R"(([\{\}\[\]\(\)]))";
        lang_java.m_rules.push_back({
            pat_java_punct, boost::regex(pat_java_punct), "\x0F$1\x10"
        });

        cache["java"] = lang_java;

        // ---------------------------------------------------------------------
        // 7. PHP Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_php;
        lang_php.m_name = "php";

        std::string pat_php_string = R"(CN([^\"]*\"|\'[^\']*\'))";
        lang_php.m_rules.push_back({
            pat_php_string, boost::regex(pat_php_string), "\x01$1\x02"
        });

        std::string pat_php_comment = R"((//[^\n]*|#[^\n]*|/\*[\s\S]*?\*/))";
        lang_php.m_rules.push_back({
            pat_php_comment, boost::regex(pat_php_comment), "\x03$1\x04"
        });

        std::string pat_php_var = R"((\$[a-zA-Z_]\w*))";
        lang_php.m_rules.push_back({
            pat_php_var, boost::regex(pat_php_var), "\x07$1\x08"
        });

        std::string pat_php_keyword = R"(\b(function|class|public|private|protected|return|if|)";
        pat_php_keyword += R"(else|elseif|for|foreach|while|echo|namespace|use|new|as)\b)";
        lang_php.m_rules.push_back({
            pat_php_keyword, boost::regex(pat_php_keyword), "\x05$1\x06"
        });

        std::string pat_php_method = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_php.m_rules.push_back({
            pat_php_method, boost::regex(pat_php_method), "\x13$1\x14"
        });

        std::string pat_php_punct = R"(([\{\}\[\]\(\)]))";
        lang_php.m_rules.push_back({
            pat_php_punct, boost::regex(pat_php_punct), "\x0F$1\x10"
        });

        cache["php"] = lang_php;

        // ---------------------------------------------------------------------
        // 8. Rust Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_rust;
        lang_rust.m_name = "rust";

        std::string pat_rust_string = R"(("[^"]*"))";
        lang_rust.m_rules.push_back({
            pat_rust_string, boost::regex(pat_rust_string), "\x01$1\x02"
        });

        std::string pat_rust_comment = R"((//[^\n]*|/\*[\s\S]*?\*/))";
        lang_rust.m_rules.push_back({
            pat_rust_comment, boost::regex(pat_rust_comment), "\x03$1\x04"
        });

        std::string pat_rust_keyword = R"(\b(fn|let|mut|match|if|else|for|while|loop|return|)";
        pat_rust_keyword += R"(struct|enum|impl|trait|pub|use|mod|move|async|await|true|false)\b)";
        lang_rust.m_rules.push_back({
            pat_rust_keyword, boost::regex(pat_rust_keyword), "\x05$1\x06"
        });

        std::string pat_rust_macro = R"(\b([a-zA-Z_]\w*!))";
        lang_rust.m_rules.push_back({
            pat_rust_macro, boost::regex(pat_rust_macro), "\x07$1\x08"
        });

        std::string pat_rust_method = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_rust.m_rules.push_back({
            pat_rust_method, boost::regex(pat_rust_method), "\x13$1\x14"
        });

        std::string pat_rust_punct = R"(([\{\}\[\]\(\)]))";
        lang_rust.m_rules.push_back({
            pat_rust_punct, boost::regex(pat_rust_punct), "\x0F$1\x10"
        });

        cache["rust"] = lang_rust;
        cache["rs"] = lang_rust;

        // ---------------------------------------------------------------------
        // 9. Kotlin Language Context
        // ---------------------------------------------------------------------
        LanguageSyntax lang_kotlin;
        lang_kotlin.m_name = "kotlin";

        std::string pat_kot_string = R"(("[^"]*"|'[^']*'))";
        lang_kotlin.m_rules.push_back({
            pat_kot_string, boost::regex(pat_kot_string), "\x01$1\x02"
        });

        std::string pat_kot_comment = R"((//[^\n]*|/\*[\s\S]*?\*/))";
        lang_kotlin.m_rules.push_back({
            pat_kot_comment, boost::regex(pat_kot_comment), "\x03$1\x04"
        });

        std::string pat_kot_keyword = R"(\b(fun|val|var|class|interface|object|return|if|else|)";
        pat_kot_keyword += R"(for|while|when|import|package|public|private|protected|internal|)";
        pat_kot_keyword += R"(this|super|null|true|false)\b)";
        lang_kotlin.m_rules.push_back({
            pat_kot_keyword, boost::regex(pat_kot_keyword), "\x05$1\x06"
        });

        std::string pat_kot_method = R"(\b([a-zA-Z_]\w*)\s*(?=\())";
        lang_kotlin.m_rules.push_back({
            pat_kot_method, boost::regex(pat_kot_method), "\x13$1\x14"
        });

        std::string pat_kot_punct = R"(([\{\}\[\]\(\)]))";
        lang_kotlin.m_rules.push_back({
            pat_kot_punct, boost::regex(pat_kot_punct), "\x0F$1\x10"
        });

        cache["kotlin"] = lang_kotlin;
        cache["kt"] = lang_kotlin;

        return cache;
    }();

    m_language_map = builtin_cache;
}

} // namespace malama::engine::markdown

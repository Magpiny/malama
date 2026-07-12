// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/parser_factory.cpp
// Purpose:     Translates trailing extensions into functional target parsers
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All神ights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/parser_factory.hpp"

#include <algorithm>
#include <filesystem>

#include "engine/storage/parsers/docx_parser.hpp"
#include "engine/storage/parsers/epub_parser.hpp"
#include "engine/storage/parsers/image_parser.hpp"
#include "engine/storage/parsers/ods_parser.hpp"
#include "engine/storage/parsers/odt_parser.hpp"
#include "engine/storage/parsers/pdf_parser.hpp"
#include "engine/storage/parsers/plain_text_parser.hpp"
#include "engine/storage/parsers/xlsx_parser.hpp"
#include "engine/storage/parsers/xml_parser.hpp"

namespace malama::engine::storage::parsers {

auto ParserFactory::resolve_parser(const std::string &file_path) noexcept
    -> std::unique_ptr<ITextParser> {
    const std::filesystem::path system_path(file_path);
    std::string file_extension = system_path.extension().string();

    std::ranges::transform(file_extension, file_extension.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });

    if (file_extension == ".pdf") {
        return std::make_unique<PdfParser>();
    }
    if (file_extension == ".docx") {
        return std::make_unique<DocxParser>();
    }
    if (file_extension == ".odt") {
        return std::make_unique<OdtParser>();
    }
    if (file_extension == ".epub") {
        return std::make_unique<EpubParser>();
    }
    if (file_extension == ".xlsx") {
        return std::make_unique<XlsxParser>();
    }
    if (file_extension == ".ods") {
        return std::make_unique<OdsParser>();
    }
    if (file_extension == ".xml") {
        return std::make_unique<XmlParser>();
    }
    if (file_extension == ".png" || file_extension == ".jpg" || file_extension == ".jpeg" ||
        file_extension == ".webp") {
        return std::make_unique<ImageParser>();
    }

    return std::make_unique<PlainTextParser>();
}

}  // namespace malama::engine::storage::parsers

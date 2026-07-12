// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/attachment_manager.cpp
// Purpose:     Integrated universal asset parsing entry point via Factory routing
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-08
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#include "engine/storage/attachment_manager.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "engine/storage/parsers/parser_factory.hpp"

namespace malama::engine::storage {

inline constexpr std::size_t max_image_size_bytes = 4UZ * 1024UZ * 1024UZ;
inline constexpr std::size_t max_pdf_size_bytes = 6UZ * 1024UZ * 1024UZ;
inline constexpr std::size_t max_total_estimated_tokens = 32000UZ;
inline constexpr std::size_t approx_chars_per_token = 4UZ;

inline constexpr std::size_t max_attachment_count = 6UZ;
inline constexpr std::size_t tokens_per_vision_image = 1024UZ;

[[nodiscard]] static auto classify_attachment_type(
    const std::filesystem::path &extension_path) noexcept -> AttachmentType {
    static constexpr std::array<std::string_view, 4UZ> image_extensions{".png", ".jpg", ".jpeg",
                                                                        ".webp"};

    std::string extension_lower = extension_path.string();
    std::ranges::transform(extension_lower, extension_lower.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });

    const bool is_image = std::ranges::any_of(
        image_extensions, [&](std::string_view candidate) { return candidate == extension_lower; });

    return is_image ? AttachmentType::IMAGE : AttachmentType::TEXT_DOCUMENT;
}

auto AttachmentManager::AnalyzeAndAdd(const std::string &file_path) noexcept
    -> std::expected<void, IngestionError> {
    std::error_code fs_error;

    if (!std::filesystem::exists(file_path, fs_error) || fs_error) {
        return std::unexpected(IngestionError::FILE_NOT_FOUND);
    }

    if (m_pending_attachments.size() >= max_attachment_count) {
        return std::unexpected(IngestionError::MAX_LIMIT_REACHED);
    }

    const auto file_size = std::filesystem::file_size(file_path, fs_error);
    if (fs_error) {
        return std::unexpected(IngestionError::READ_FAULT);
    }

    const std::filesystem::path path_view(file_path);
    std::string extension_lower = path_view.extension().string();
    std::ranges::transform(extension_lower, extension_lower.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });

    const AttachmentType type = classify_attachment_type(path_view.extension());

    if (type == AttachmentType::IMAGE && file_size > max_image_size_bytes) {
        return std::unexpected(IngestionError::IMAGE_TOO_LARGE);
    }

    if (extension_lower == ".pdf" && file_size > max_pdf_size_bytes) {
        return std::unexpected(IngestionError::DOCUMENT_TOO_LARGE);
    }

    std::size_t estimated_tokens = tokens_per_vision_image;
    std::optional<std::string> cached_text;

    if (type == AttachmentType::TEXT_DOCUMENT) {
        // Parse now, before deciding whether this attachment fits the
        // context budget. Raw on-disk file size is a poor proxy for token
        // count on binary/compressed formats (PDF, DOCX, XLSX, EPUB) --
        // most of those bytes are fonts, images, and container overhead,
        // not text. Budgeting on the actual extracted text is accurate;
        // it also means ExtractTextContent() never has to re-parse this
        // file at send-time, since the result is cached below.
        auto text_parser = parsers::ParserFactory::resolve_parser(file_path);
        if (text_parser == nullptr) {
            return std::unexpected(IngestionError::PARSING_FAILED);
        }

        auto processing_output = text_parser->parse_file(file_path);
        if (processing_output.m_error_code != parsers::ParserErrorCode::SUCCESS) {
            return std::unexpected(IngestionError::PARSING_FAILED);
        }

        estimated_tokens = processing_output.m_extracted_text.size() / approx_chars_per_token;
        cached_text = std::move(processing_output.m_extracted_text);
    }

    std::size_t running_total_tokens = estimated_tokens;
    for (const auto &pending : m_pending_attachments) {
        running_total_tokens += pending.m_estimated_tokens;
    }
    if (running_total_tokens > max_total_estimated_tokens) {
        // cached_text (if any) is simply discarded here -- the file was
        // parsed to get an accurate size, but never queued, so there's
        // nothing left holding it once this function returns.
        return std::unexpected(IngestionError::CONTEXT_OVERFLOW);
    }

    m_pending_attachments.push_back(AttachmentInfo{
        .m_file_path = file_path,
        .m_file_name = path_view.filename().string(),
        .m_type = type,
        .m_size_bytes = file_size,
        .m_estimated_tokens = estimated_tokens,
        .m_cached_text = std::move(cached_text),
    });

    return {};
}

auto AttachmentManager::RemoveByIndex(std::size_t file_index) noexcept -> void {
    if (file_index < m_pending_attachments.size()) {
        m_pending_attachments.erase(m_pending_attachments.begin() +
                                    static_cast<std::ptrdiff_t>(file_index));
    }
}

auto AttachmentManager::GetPendingAttachments() const noexcept
    -> const std::vector<AttachmentInfo> & {
    return m_pending_attachments;
}

auto AttachmentManager::ClearQueue() noexcept -> void {
    m_pending_attachments.clear();
}

// Parsing already happened in AnalyzeAndAdd -- this is now a pure lookup
// against info.m_cached_text, which is why it can stay static (it never
// touches *this). Returns nullopt for IMAGE attachments, which never
// populate the cache, and for any TEXT_DOCUMENT attachment somehow
// constructed outside AnalyzeAndAdd (there shouldn't be any).
auto AttachmentManager::ExtractTextContent(const AttachmentInfo &info) noexcept
    -> std::optional<std::string> {
    return info.m_cached_text;
}

}  // namespace malama::engine::storage

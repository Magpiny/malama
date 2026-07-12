// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/attachment_manager.hpp
// Purpose:     Zero-copy high-performance attachment processing subsystem
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace malama::engine::storage {

/// @brief Classification metric tracking structural attachment variants.
enum class AttachmentType : std::uint8_t { TEXT_DOCUMENT, IMAGE };

/// @brief Detailed localized validation error boundaries.
enum class IngestionError : std::uint8_t {
    FILE_NOT_FOUND,
    IMAGE_TOO_LARGE,
    DOCUMENT_TOO_LARGE,
    READ_FAULT,
    CONTEXT_OVERFLOW,
    PARSING_FAILED,
    MAX_LIMIT_REACHED
};

/// @brief Metadata payload block tracking processed attachment files.
struct AttachmentInfo {
    std::string m_file_path;
    std::string m_file_name;
    AttachmentType m_type;
    std::size_t m_size_bytes{0};
    std::size_t m_estimated_tokens{0};
    std::optional<std::string> m_cached_text;
};

class AttachmentManager final {
   public:
    AttachmentManager() = default;
    ~AttachmentManager() = default;

    AttachmentManager(const AttachmentManager &) = default;
    auto operator=(const AttachmentManager &) -> AttachmentManager & = default;
    AttachmentManager(AttachmentManager &&) noexcept = default;
    auto operator=(AttachmentManager &&) noexcept -> AttachmentManager & = default;

    [[nodiscard]] auto AnalyzeAndAdd(const std::string &file_path) noexcept
        -> std::expected<void, IngestionError>;

    [[nodiscard]] auto GetPendingAttachments() const noexcept
        -> const std::vector<AttachmentInfo> &;

    auto ClearQueue() noexcept -> void;

    [[nodiscard]] static auto ExtractTextContent(const AttachmentInfo &info) noexcept
        -> std::optional<std::string>;

    auto RemoveByIndex(std::size_t index) noexcept -> void;

   private:
    std::vector<AttachmentInfo> m_pending_attachments;
};

}  // namespace malama::engine::storage

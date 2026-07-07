// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/attachment_manager.hpp
// Purpose:     Zero-copy high-performance attachment processing subsystem
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace malama::engine::storage {

/// @brief Classification metric tracking structural attachment variants.
enum class AttachmentType : std::uint8_t { TEXT_DOCUMENT, IMAGE };

/// @brief Detailed localized validation error boundaries.
enum class IngestionError : std::uint8_t {
    FILE_NOT_FOUND,
    IMAGE_TOO_LARGE,
    READ_FAULT,
    CONTEXT_OVERFLOW
};

/// @brief Metadata payload block tracking processed attachment files.
struct AttachmentInfo {
    std::string m_file_path;
    std::string m_file_name;
    AttachmentType m_type;
    std::size_t m_size_bytes{0};
    std::size_t m_estimated_tokens{0};
};

/// @brief Manages data ingestion pipelines for large text files and local visual formats.
class AttachmentManager final {
   public:
    explicit AttachmentManager() = default;
    ~AttachmentManager() = default;

    AttachmentManager(const AttachmentManager &) = delete;
    auto operator=(const AttachmentManager &) -> AttachmentManager & = delete;
    AttachmentManager(AttachmentManager &&) noexcept = default;
    auto operator=(AttachmentManager &&) noexcept -> AttachmentManager & = default;

    /// @brief Pre-flight registers and analyzes a target file path.
    /// @param file_path Absolute file location targeting ingestion.
    /// @return Meta information on success, structured error flag on failure.
    [[nodiscard]] auto AnalyzeAndAdd(const std::string &file_path) noexcept
        -> std::expected<AttachmentInfo, IngestionError>;

    /// @brief Erases all current pending files inside the attachment workspace tray.
    void ClearQueue() noexcept;

    /// @brief Memory-maps and extracts targeted text files safely as injected context blocks.
    /// @param info Structural description metadata block of the file target.
    /// @return Content string layout on success, structural enum fault on failure.
    [[nodiscard]] auto ExtractTextContent(const AttachmentInfo &info) noexcept
        -> std::expected<std::string, IngestionError>;

    [[nodiscard]] auto GetPendingAttachments() const noexcept
        -> const std::vector<AttachmentInfo> &;

    [[nodiscard]] auto ComputeTotalEstimatedTokens() const noexcept -> std::size_t;

   private:
    std::vector<AttachmentInfo> m_pending_tray;
};

}  // namespace malama::engine::storage

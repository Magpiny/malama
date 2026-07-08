// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/attachment_manager.cpp
// Purpose:     Implements zero-copy memory maps and token estimation calculations
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#include "engine/storage/attachment_manager.hpp"

#include <boost/iostreams/device/mapped_file.hpp>
#include <common/constants.hpp>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace malama::engine::storage {

namespace fs = std::filesystem;

auto AttachmentManager::AnalyzeAndAdd(const std::string &file_path) noexcept
    -> std::expected<AttachmentInfo, IngestionError> {
    std::error_code err_code;
    if (!fs::exists(file_path, err_code)) {
        spdlog::warn("Attachment target missing: {}", file_path);
        return std::unexpected(IngestionError::FILE_NOT_FOUND);
    }

    const auto size_bytes = fs::file_size(file_path, err_code);
    if (err_code) {
        return std::unexpected(IngestionError::READ_FAULT);
    }

    auto target_type = AttachmentType::TEXT_DOCUMENT;
    const auto extension = fs::path(file_path).extension().string();

    if (extension == ".jpeg" || extension == ".jpg" || extension == ".png") {
        target_type = AttachmentType::IMAGE;
        // Strictly enforce your 4MB vision VRAM protection constraint ceiling
        if (size_bytes > 4 * constants::absolute_max_buffer_bytes) {
            spdlog::error("Visual asset size boundary violation: {} bytes", size_bytes);
            return std::unexpected(IngestionError::IMAGE_TOO_LARGE);
        }
    }

    AttachmentInfo info;
    info.m_file_path = file_path;
    info.m_file_name = fs::path(file_path).filename().string();
    info.m_type = target_type;
    info.m_size_bytes = size_bytes;

    // Fast character-to-token heuristic (1 token approx 4 chars for code/logs)
    switch (target_type) {
        case AttachmentType::TEXT_DOCUMENT: {
            info.m_estimated_tokens = size_bytes / 4;
            break;
        }
        case AttachmentType::IMAGE: {
            // Local vision models allocate static visual context tokens per block layer
            info.m_estimated_tokens = 1024;
            break;
        }
    }

    m_pending_tray.push_back(info);
    return info;
}

void AttachmentManager::ClearQueue() noexcept {
    m_pending_tray.clear();
}

auto AttachmentManager::ExtractTextContent(const AttachmentInfo &info) noexcept
    -> std::expected<std::string, IngestionError> {
    // Block image assets from accidental raw string injection pipelines
    switch (info.m_type) {
        case AttachmentType::IMAGE: {
            return std::unexpected(IngestionError::READ_FAULT);
        }
        case AttachmentType::TEXT_DOCUMENT: {
            break;
        }
    }

    try {
        // High-speed Linux kernel cache direct allocation via zero-copy mapped files
        boost::iostreams::mapped_file_source mmap_file(info.m_file_path);
        if (!mmap_file.is_open()) {
            return std::unexpected(IngestionError::READ_FAULT);
        }

        std::string structural_payload;
        structural_payload.reserve(mmap_file.size() + info.m_file_name.size() + 64);

        structural_payload += "\n--- START DOCUMENT: ";
        structural_payload += info.m_file_name;
        structural_payload += " ---\n";
        structural_payload += std::string(mmap_file.data(), mmap_file.size());
        structural_payload += "\n--- END DOCUMENT: ";
        structural_payload += info.m_file_name;
        structural_payload += " ---\n";

        return structural_payload;
    } catch (const std::exception &ex) {
        spdlog::error("Zero-copy stream allocation fault: {}", ex.what());
        return std::unexpected(IngestionError::READ_FAULT);
    }
}

auto AttachmentManager::GetPendingAttachments() const noexcept
    -> const std::vector<AttachmentInfo> & {
    return m_pending_tray;
}

auto AttachmentManager::ComputeTotalEstimatedTokens() const noexcept -> std::size_t {
    std::size_t total_sum = 0;
    for (const auto &info : m_pending_tray) {
        total_sum += info.m_estimated_tokens;
    }
    return total_sum;
}

}  // namespace malama::engine::storage

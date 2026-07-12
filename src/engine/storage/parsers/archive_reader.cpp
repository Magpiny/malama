// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/parsers/archive_reader.cpp
// Purpose:     Libarchive implementation for parsing wrapped XML targets securely
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/parsers/archive_reader.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <array>

namespace malama::engine::storage::parsers {

inline constexpr std::size_t internal_read_block_size = 16384UZ;

auto ArchiveReader::extract_file_to_buffer(const std::string &archive_path,
                                           const std::string &target_inner_file) noexcept
    -> std::optional<std::vector<char>> {
    struct archive *archive_handle = archive_read_new();
    if (archive_handle == nullptr) {
        return std::nullopt;
    }

    archive_read_support_filter_all(archive_handle);
    archive_read_support_format_all(archive_handle);

    if (archive_read_open_filename(archive_handle, archive_path.c_str(),
                                   internal_read_block_size) != ARCHIVE_OK) {
        archive_read_free(archive_handle);
        return std::nullopt;
    }

    struct archive_entry *entry_ptr = nullptr;
    std::vector<char> output_buffer;
    bool item_discovered = false;

    while (archive_read_next_header(archive_handle, &entry_ptr) == ARCHIVE_OK) {
        const std::string current_entry_name(archive_entry_pathname(entry_ptr));
        if (current_entry_name == target_inner_file) {
            item_discovered = true;

            while (true) {
                std::array<char, internal_read_block_size> local_chunk{};
                const la_ssize_t read_bytes =
                    archive_read_data(archive_handle, local_chunk.data(), internal_read_block_size);

                if (read_bytes < 0) {
                    archive_read_free(archive_handle);
                    return std::nullopt;
                }
                if (read_bytes == 0) {
                    break;
                }

                output_buffer.insert(output_buffer.end(), local_chunk.begin(),
                                     local_chunk.begin() + static_cast<std::ptrdiff_t>(read_bytes));
            }
            break;
        }
    }

    archive_read_free(archive_handle);
    return item_discovered ? std::make_optional(output_buffer) : std::nullopt;
}

}  // namespace malama::engine::storage::parsers

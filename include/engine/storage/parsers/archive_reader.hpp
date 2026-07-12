// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/parsers/archive_reader.hpp
// Purpose:     In-memory decompression reader abstraction utilizing libarchive
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include <optional>
#include <string>
#include <vector>

namespace malama::engine::storage::parsers {

class ArchiveReader final {
   public:
    ArchiveReader() = default;
    ArchiveReader(const ArchiveReader &) = default;
    ArchiveReader(ArchiveReader &&) = delete;
    ArchiveReader &operator=(const ArchiveReader &) = default;
    ArchiveReader &operator=(ArchiveReader &&) = delete;
    ~ArchiveReader() = default;

    [[nodiscard]] auto extract_file_to_buffer(const std::string &archive_path,
                                              const std::string &target_inner_file) noexcept
        -> std::optional<std::vector<char>>;
};

}  // namespace malama::engine::storage::parsers

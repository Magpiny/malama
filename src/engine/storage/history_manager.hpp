// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/history_manager.hpp
// Purpose:     Thread-serialized chat history management engine via SQLite
// Author:      Wanjare S. <samuewanjare@protonmail.com>
// Created:     2026-06-11
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "core/models.hpp"

struct sqlite3;  // Opaque forward pointer configuration to block internal leaks

namespace malama::engine::storage {

class HistoryManager final {
   public:
    // Initializes directory frames and provisions the synchronized SQLite database
    explicit HistoryManager(std::filesystem::path storage_dir);
    ~HistoryManager();

    // Enforce definitive operational boundaries for underlying handle resources
    HistoryManager(const HistoryManager &) = delete;
    auto operator=(const HistoryManager &) -> HistoryManager & = delete;
    HistoryManager(HistoryManager &&) noexcept = delete;
    auto operator=(HistoryManager &&) noexcept -> HistoryManager & = delete;

    // Lifecycle Management
    [[nodiscard]] auto CreateSession(const std::string &initial_title) -> core::SessionMetadata;
    auto DeleteSession(const std::string &session_id) -> void;

    // Transactional Serialized Database Mutations
    auto AppendMessage(const std::string &session_id, const core::Message &message) -> void;
    auto UpdateSessionTitle(const std::string &session_id, const std::string &new_title) -> void;
    auto ToggleSessionPin(const std::string &session_id) -> void;
    auto ToggleMessageStar(const std::string &session_id, const std::string &message_id) -> void;
    auto UpdateSessionParameters(const std::string &session_id, const core::ModelParameters &params)
        -> void;

    // Data Retrieval Pipelines
    [[nodiscard]] auto LoadAllMetadata() const -> std::vector<core::SessionMetadata>;
    [[nodiscard]] auto LoadSession(const std::string &session_id) const
        -> std::optional<core::ChatSession>;

    [[nodiscard]] static auto GenerateUuidString() -> std::string;
    [[nodiscard]] static auto GetCurrentEpoch() -> uint64_t;

   private:
    auto InitializeDatabase() noexcept -> bool;

    std::filesystem::path m_storage_dir;
    std::filesystem::path m_db_path;
    sqlite3 *m_db_handle{nullptr};

    // Mutex lock to synchronize UI inputs and async streaming background workers
    mutable std::mutex m_mutex;
};

}  // namespace malama::engine::storage

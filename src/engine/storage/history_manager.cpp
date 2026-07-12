// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/history_manager.cpp
// Purpose:     Implements thread-safe, transaction-locked queries for history
// Author:      Wanjare S. <samuewanjare@protonmail.com>
// Created:     2026-06-11
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/storage/history_manager.hpp"

#include <algorithm>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <sqlite3.h>

namespace malama::engine::storage {

HistoryManager::HistoryManager(std::filesystem::path storage_dir)
    : m_storage_dir(std::move(storage_dir)) {
    if (!std::filesystem::exists(m_storage_dir)) {
        std::filesystem::create_directories(m_storage_dir);
    }
    m_db_path = m_storage_dir / "history.db";
    InitializeDatabase();
}

auto HistoryManager::GenerateUuidString() -> std::string {
    static boost::uuids::random_generator generator;
    return boost::uuids::to_string(generator());
}

auto HistoryManager::GetCurrentEpoch() -> uint64_t {
    auto time_point = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(time_point).count());
}

auto HistoryManager::InitializeDatabase() noexcept -> bool {
    std::lock_guard<std::mutex> lock(m_mutex);

    int result_code = sqlite3_open(m_db_path.c_str(), &m_db_handle);
    if (result_code != SQLITE_OK) {
        return false;
    }

    const char *sql_sessions =
        "CREATE TABLE IF NOT EXISTS sessions ("
        "session_id TEXT PRIMARY KEY, "
        "title TEXT, "
        "created_at INTEGER, "
        "updated_at INTEGER, "
        "is_pinned INTEGER DEFAULT 0);";

    char *error_message = nullptr;
    result_code = sqlite3_exec(m_db_handle, sql_sessions, nullptr, nullptr, &error_message);
    if (result_code != SQLITE_OK) {
        sqlite3_free(error_message);
        return false;
    }

    const char *sql_messages =
        "CREATE TABLE IF NOT EXISTS messages ("
        "message_id TEXT PRIMARY KEY, "
        "session_id TEXT, "
        "role INTEGER, "
        "content TEXT, "
        "timestamp INTEGER, "
        "is_starred INTEGER DEFAULT 0, "
        "FOREIGN KEY(session_id) REFERENCES sessions(session_id) ON DELETE CASCADE);";

    result_code = sqlite3_exec(m_db_handle, sql_messages, nullptr, nullptr, &error_message);
    if (result_code != SQLITE_OK) {
        sqlite3_free(error_message);
        return false;
    }
    return true;
}

auto HistoryManager::CreateSession(const std::string &initial_title) -> core::SessionMetadata {
    std::lock_guard<std::mutex> lock(m_mutex);

    core::SessionMetadata metadata;
    metadata.m_session_id = GenerateUuidString();
    metadata.m_title = initial_title;
    metadata.m_created_at = GetCurrentEpoch();
    metadata.m_updated_at = metadata.m_created_at;
    metadata.m_is_pinned = false;

    if (m_db_handle == nullptr) {
        return metadata;
    }

    const char *sql_insert =
        "INSERT INTO sessions (session_id, title, created_at, updated_at, is_pinned) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *statement_ptr = nullptr;
    int result_code = sqlite3_prepare_v2(m_db_handle, sql_insert, -1, &statement_ptr, nullptr);
    if (result_code == SQLITE_OK) {
        sqlite3_bind_text(statement_ptr, 1, metadata.m_session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement_ptr, 2, metadata.m_title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(statement_ptr, 3, static_cast<sqlite3_int64>(metadata.m_created_at));
        sqlite3_bind_int64(statement_ptr, 4, static_cast<sqlite3_int64>(metadata.m_updated_at));
        sqlite3_bind_int(statement_ptr, 5, metadata.m_is_pinned ? 1 : 0);
        sqlite3_step(statement_ptr);
        sqlite3_finalize(statement_ptr);
    }
    return metadata;
}

auto HistoryManager::DeleteSession(const std::string &session_id) -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db_handle == nullptr) {
        return;
    }

    const char *sql_delete = "DELETE FROM sessions WHERE session_id = ?;";
    sqlite3_stmt *statement_ptr = nullptr;
    int result_code = sqlite3_prepare_v2(m_db_handle, sql_delete, -1, &statement_ptr, nullptr);
    if (result_code == SQLITE_OK) {
        sqlite3_bind_text(statement_ptr, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(statement_ptr);
        sqlite3_finalize(statement_ptr);
    }
}

auto HistoryManager::UpdateSessionTitle(const std::string &session_id, const std::string &new_title)
    -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db_handle == nullptr) {
        return;
    }

    const char *sql_update = "UPDATE sessions SET title = ?, updated_at = ? WHERE session_id = ?;";
    sqlite3_stmt *statement_ptr = nullptr;
    int result_code = sqlite3_prepare_v2(m_db_handle, sql_update, -1, &statement_ptr, nullptr);
    if (result_code == SQLITE_OK) {
        sqlite3_bind_text(statement_ptr, 1, new_title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(statement_ptr, 2, static_cast<sqlite3_int64>(GetCurrentEpoch()));
        sqlite3_bind_text(statement_ptr, 3, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(statement_ptr);
        sqlite3_finalize(statement_ptr);
    }
}

auto HistoryManager::ToggleSessionPin(const std::string &session_id) -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db_handle == nullptr) {
        return;
    }

    const char *sql_toggle = "UPDATE sessions SET is_pinned = NOT is_pinned WHERE session_id = ?;";
    sqlite3_stmt *statement_ptr = nullptr;
    int result_code = sqlite3_prepare_v2(m_db_handle, sql_toggle, -1, &statement_ptr, nullptr);
    if (result_code == SQLITE_OK) {
        sqlite3_bind_text(statement_ptr, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(statement_ptr);
        sqlite3_finalize(statement_ptr);
    }
}

auto HistoryManager::AppendMessage(const std::string &session_id, const core::Message &message)
    -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db_handle == nullptr) {
        return;
    }

    const char *sql_insert =
        "INSERT INTO messages (message_id, session_id, role, content, timestamp, is_starred) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *statement_ptr = nullptr;
    int result_code = sqlite3_prepare_v2(m_db_handle, sql_insert, -1, &statement_ptr, nullptr);
    if (result_code == SQLITE_OK) {
        std::string msg_id = message.m_id.empty() ? GenerateUuidString() : message.m_id;
        sqlite3_bind_text(statement_ptr, 1, msg_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement_ptr, 2, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(statement_ptr, 3, static_cast<int>(message.m_role));
        sqlite3_bind_text(statement_ptr, 4, message.m_content.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(statement_ptr, 5, static_cast<sqlite3_int64>(message.m_timestamp));
        sqlite3_bind_int(statement_ptr, 6, message.m_is_starred ? 1 : 0);
        sqlite3_step(statement_ptr);
        sqlite3_finalize(statement_ptr);
    }

    const char *sql_touch = "UPDATE sessions SET updated_at = ? WHERE session_id = ?;";
    if (sqlite3_prepare_v2(m_db_handle, sql_touch, -1, &statement_ptr, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(statement_ptr, 1, static_cast<sqlite3_int64>(GetCurrentEpoch()));
        sqlite3_bind_text(statement_ptr, 2, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(statement_ptr);
        sqlite3_finalize(statement_ptr);
    }
}

auto HistoryManager::ToggleMessageStar(const std::string &session_id, const std::string &message_id)
    -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db_handle == nullptr) {
        return;
    }

    const char *sql_star =
        "UPDATE messages SET is_starred = NOT is_starred "
        "WHERE session_id = ? AND message_id = ?;";
    sqlite3_stmt *statement_ptr = nullptr;
    int result_code = sqlite3_prepare_v2(m_db_handle, sql_star, -1, &statement_ptr, nullptr);
    if (result_code == SQLITE_OK) {
        sqlite3_bind_text(statement_ptr, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement_ptr, 2, message_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(statement_ptr);
        sqlite3_finalize(statement_ptr);
    }
}

auto HistoryManager::LoadAllMetadata() const -> std::vector<core::SessionMetadata> {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<core::SessionMetadata> metadata_list;
    if (m_db_handle == nullptr) {
        return metadata_list;
    }

    const char *sql_select =
        "SELECT session_id, title, created_at, updated_at, is_pinned FROM sessions;";
    sqlite3_stmt *statement_ptr = nullptr;
    int result_code = sqlite3_prepare_v2(m_db_handle, sql_select, -1, &statement_ptr, nullptr);
    if (result_code == SQLITE_OK) {
        while (sqlite3_step(statement_ptr) == SQLITE_ROW) {
            core::SessionMetadata metadata;
            metadata.m_session_id =
                reinterpret_cast<const char *>(sqlite3_column_text(statement_ptr, 0));
            metadata.m_title =
                reinterpret_cast<const char *>(sqlite3_column_text(statement_ptr, 1));
            metadata.m_created_at = static_cast<uint64_t>(sqlite3_column_int64(statement_ptr, 2));
            metadata.m_updated_at = static_cast<uint64_t>(sqlite3_column_int64(statement_ptr, 3));
            metadata.m_is_pinned = sqlite3_column_int(statement_ptr, 4) != 0;
            metadata_list.push_back(metadata);
        }
        sqlite3_finalize(statement_ptr);
    }
    return metadata_list;
}

auto HistoryManager::LoadSession(const std::string &session_id) const
    -> std::optional<core::ChatSession> {
    std::lock_guard<std::mutex> lock(m_mutex);
    core::ChatSession session_obj;
    bool session_exists = false;

    if (m_db_handle == nullptr) {
        return std::nullopt;
    }

    const char *sql_session =
        "SELECT session_id, title, created_at, updated_at, is_pinned "
        "FROM sessions WHERE session_id = ?;";
    sqlite3_stmt *statement_ptr = nullptr;
    int result_code = sqlite3_prepare_v2(m_db_handle, sql_session, -1, &statement_ptr, nullptr);
    if (result_code == SQLITE_OK) {
        sqlite3_bind_text(statement_ptr, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(statement_ptr) == SQLITE_ROW) {
            session_obj.m_metadata.m_session_id =
                reinterpret_cast<const char *>(sqlite3_column_text(statement_ptr, 0));
            session_obj.m_metadata.m_title =
                reinterpret_cast<const char *>(sqlite3_column_text(statement_ptr, 1));
            session_obj.m_metadata.m_created_at =
                static_cast<uint64_t>(sqlite3_column_int64(statement_ptr, 2));
            session_obj.m_metadata.m_updated_at =
                static_cast<uint64_t>(sqlite3_column_int64(statement_ptr, 3));
            session_obj.m_metadata.m_is_pinned = sqlite3_column_int(statement_ptr, 4) != 0;
            session_exists = true;
        }
        sqlite3_finalize(statement_ptr);
    }

    if (!session_exists) {
        return std::nullopt;
    }

    const char *sql_messages =
        "SELECT message_id, role, content, timestamp, is_starred "
        "FROM messages WHERE session_id = ? ORDER BY timestamp ASC;";
    result_code = sqlite3_prepare_v2(m_db_handle, sql_messages, -1, &statement_ptr, nullptr);
    if (result_code == SQLITE_OK) {
        sqlite3_bind_text(statement_ptr, 1, session_id.c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(statement_ptr) == SQLITE_ROW) {
            core::Message message_obj;
            message_obj.m_id =
                reinterpret_cast<const char *>(sqlite3_column_text(statement_ptr, 0));
            message_obj.m_role =
                static_cast<core::MessageRole>(sqlite3_column_int(statement_ptr, 1));
            message_obj.m_content =
                reinterpret_cast<const char *>(sqlite3_column_text(statement_ptr, 2));
            message_obj.m_timestamp = static_cast<uint64_t>(sqlite3_column_int64(statement_ptr, 3));
            message_obj.m_is_starred = sqlite3_column_int(statement_ptr, 4) != 0;
            session_obj.m_messages.push_back(message_obj);
        }
        sqlite3_finalize(statement_ptr);
    }
    return session_obj;
}

HistoryManager::~HistoryManager() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db_handle != nullptr) {
        sqlite3_close(m_db_handle);
        m_db_handle = nullptr;
    }
}

}  // namespace malama::engine::storage

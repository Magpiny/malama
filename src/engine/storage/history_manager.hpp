// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/storage/history_manager.hpp
// Purpose:     Persistent chat session and history management via JSONL/Glaze
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include "core/models.hpp"
#include <filesystem>
#include <optional>
#include <vector>
#include <string>

#include "core/models.hpp"
namespace malama::engine::storage {

class HistoryManager {
public:
    // Initializes the storage directory (e.g., ~/.local/share/malama/sessions/)
    explicit HistoryManager(std::filesystem::path storage_dir);

    // Lifecycle Management
    [[nodiscard]] auto CreateSession(const std::string& initial_title) -> core::SessionMetadata;
    auto DeleteSession(const std::string& session_id) -> void;

    // Zero-Overhead File Mutations
    auto AppendMessage(const std::string& session_id, const core::Message& message) -> void;
    auto UpdateSessionTitle(const std::string& session_id, const std::string& new_title) -> void;
    auto ToggleSessionPin(const std::string& session_id) -> void;
    auto ToggleMessageStar(const std::string& session_id, const std::string& message_id) -> void;

    // Data Retrieval
    [[nodiscard]] auto LoadAllMetadata() const -> std::vector<core::SessionMetadata>;
    [[nodiscard]] auto LoadSession(const std::string& session_id) const -> std::optional<core::ChatSession>;

    [[nodiscard]] static auto GenerateUuidString() -> std::string;
    [[nodiscard]] static auto GetCurrentEpoch() -> uint64_t;


private:
    std::filesystem::path m_storage_dir;
    std::filesystem::path m_index_path;

    [[nodiscard]] auto GetSessionFilePath(const std::string& id) const -> std::filesystem::path;
   };

} // namespace malama::engine::storage

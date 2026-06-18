// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/storage/history_manager.cpp
// Purpose:     Implements fast disk I/O and JSONL parsing for chat history
// /////////////////////////////////////////////////////////////////////////////

#include "engine/storage/history_manager.hpp"
#include <fstream>
#include <chrono>
#include <algorithm>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <glaze/glaze.hpp>

namespace malama::engine::storage {

HistoryManager::HistoryManager(std::filesystem::path storage_dir)
    : m_storage_dir(std::move(storage_dir)) {
    
    // Ensure the base directory exists
    if (!std::filesystem::exists(m_storage_dir)) {
        std::filesystem::create_directories(m_storage_dir);
    }
    m_index_path = m_storage_dir / "sessions_index.json";
}

auto HistoryManager::GenerateUuidString() -> std::string {
    static boost::uuids::random_generator generator;
    return boost::uuids::to_string(generator());
}

auto HistoryManager::GetCurrentEpoch() -> uint64_t {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

auto HistoryManager::GetSessionFilePath(const std::string& id) const -> std::filesystem::path {
    return m_storage_dir / (id + ".jsonl");
}

auto HistoryManager::LoadAllMetadata() const -> std::vector<core::SessionMetadata> {
    std::vector<core::SessionMetadata> sessions;
    if (std::filesystem::exists(m_index_path)) {
        std::ifstream in(m_index_path);
        if (in.is_open()) {
            std::stringstream buffer;
            buffer << in.rdbuf();
            // Cast to void to satisfy your strict [[nodiscard]] compiler flag!
            (void)glz::read_json(sessions, buffer.str());
        }
    }
    return sessions;
}

auto HistoryManager::CreateSession(const std::string& initial_title) -> core::SessionMetadata {
    core::SessionMetadata new_session;
    new_session.m_session_id = GenerateUuidString();
    new_session.m_title = initial_title;
    new_session.m_updated_at = GetCurrentEpoch();
    new_session.m_is_pinned = false;

    // Load, Append, and Save Index
    auto sessions = LoadAllMetadata();
    sessions.push_back(new_session);
    std::string buffer;
    (void)glz::write_json(sessions, buffer); // (void) fixes the nodiscard error!
    std::ofstream out(m_index_path, std::ios::trunc);
    if (out.is_open()) { out << buffer; }

    // Create the empty .jsonl file for the actual messages
    std::ofstream touch_file(GetSessionFilePath(new_session.m_session_id));
    return new_session;
}

auto HistoryManager::DeleteSession(const std::string& session_id) -> void {
    auto sessions = LoadAllMetadata();
    std::erase_if(sessions, [&](const auto& session) {
        return session.m_session_id == session_id;
    });
    
  std::string buffer;
    (void)glz::write_json(sessions, buffer); // (void) fixes the nodiscard error!
    std::ofstream out(m_index_path, std::ios::trunc);
    if (out.is_open()) { out << buffer; } 

    // Remove the actual chat data file
    std::error_code ec;
    std::filesystem::remove(GetSessionFilePath(session_id), ec);
}

auto HistoryManager::UpdateSessionTitle(const std::string& session_id, const std::string& new_title) -> void {
    auto sessions = LoadAllMetadata();
    for (auto& session : sessions) {
        if (session.m_session_id == session_id) {
            session.m_title = new_title;
            session.m_updated_at = GetCurrentEpoch();
            break;
        }
    }
    std::string buffer;
    (void)glz::write_json(sessions, buffer); // (void) fixes the nodiscard error!
    std::ofstream out(m_index_path, std::ios::trunc);
    if (out.is_open()) { out << buffer; }
}

auto HistoryManager::ToggleSessionPin(const std::string& session_id) -> void {
    auto sessions = LoadAllMetadata();
    for (auto& session : sessions) {
        if (session.m_session_id == session_id) {
            session.m_is_pinned = !session.m_is_pinned;
            break;
        }
    }
    std::string buffer;
    (void)glz::write_json(sessions, buffer); // (void) fixes the nodiscard error!
    std::ofstream out(m_index_path, std::ios::trunc);
    if (out.is_open()) { out << buffer; }
}

// Memory Profile: O(1) - Appends string natively without loading history into RAM
auto HistoryManager::AppendMessage(const std::string& session_id, const core::Message& message) -> void {
    std::string json_str;
    (void)glz::write_json(message, json_str);

    // Open in append mode (std::ios::app)
    std::ofstream out(GetSessionFilePath(session_id), std::ios::app);
    if (out.is_open()) {
        out << json_str << "\n";
    }

    // Update the 'updated_at' timestamp in the index so it floats to the top
    auto sessions = LoadAllMetadata();
    for (auto& session : sessions) {
        if (session.m_session_id == session_id) {
            session.m_updated_at = GetCurrentEpoch();
            break;
        }
    }
    std::string buffer;
    (void)glz::write_json(sessions, buffer); // (void) fixes the nodiscard error!
    std::ofstream outp(m_index_path, std::ios::trunc);
    if (outp.is_open()) { outp << buffer; }
}

// Memory Profile: O(N) where N is number of messages. Uses stream buffering.
auto HistoryManager::LoadSession(const std::string& session_id) const -> std::optional<core::ChatSession> {
    core::ChatSession chat_session;
    
    // 1. Find Metadata
    auto sessions = LoadAllMetadata();
    auto it = std::find_if(sessions.begin(), sessions.end(), [&](const auto& s) {
        return s.m_session_id == session_id;
    });

    if (it == sessions.end()) { return std::nullopt; }
    chat_session.m_metadata = *it;

    // 2. Stream JSONL File
    std::ifstream in(GetSessionFilePath(session_id));
    if (!in.is_open()) { return chat_session; } // Return empty session if no file yet

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) { continue;
}
        
        core::Message msg;
        auto ec = glz::read_json(msg, line);
        if (!ec) {
            chat_session.m_messages.push_back(std::move(msg));
        }
    }

    return chat_session;
}

// Needed to implement inline message starring
auto HistoryManager::ToggleMessageStar(const std::string& session_id, const std::string& message_id) -> void {
    auto session_opt = LoadSession(session_id);
    if (!session_opt.has_value()) { return;
}

    auto& session = session_opt.value();
    for (auto& msg : session.m_messages) {
        if (msg.m_id == message_id) {
            msg.m_is_starred = !msg.m_is_starred;
            break;
        }
    }

    // Rewrite the JSONL file (Acceptable tradeoff since local chat logs are small files)
    std::ofstream out(GetSessionFilePath(session_id), std::ios::trunc);
    if (out.is_open()) {
        for (const auto& msg : session.m_messages) {
            std::string json_str;
            (void)glz::write_json(msg, json_str);
            out << json_str << "\n";
        }
    }
}

} // namespace malama::engine::storage

#include "../include/config/config_manager.hpp"
#include <fstream>
#include <sstream>

namespace malama::config {

auto ConfigManager::get_instance() noexcept -> ConfigManager& {
    static ConfigManager instance;
    return instance;
}

auto ConfigManager::load_config(const std::string& filepath) noexcept -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string buffer;
    if (auto err = glz::read_file_json(m_current_config, filepath, buffer)) {
        // If file doesn't exist or is invalid, it falls back to the struct defaults
    }
}

auto ConfigManager::save_config(const std::string& filepath) noexcept -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string buffer;
    [[maybe_unused]] auto err = glz::write_file_json(m_current_config, filepath, buffer);
}

auto ConfigManager::get_config() const noexcept -> AppConfig {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_current_config;
}

auto ConfigManager::update_config(const AppConfig& new_config) noexcept -> void {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_current_config = new_config;
    }
    // Notify all listeners (UI components) that settings changed
    for (const auto& observer : m_observers) {
        observer(m_current_config);
    }
}

auto ConfigManager::register_observer(observer_callback callback) noexcept -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.push_back(std::move(callback));
    // Immediately push current state to new observer
    m_observers.back()(m_current_config);
}

} // namespace malama::config

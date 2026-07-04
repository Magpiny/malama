// /////////////////////////////////////////////////////////////////////////////
// Name:        src/config/config_manager.cpp
// Purpose:     Thread-safe configuration reflection architecture implementation
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "config/config_manager.hpp"
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
    // Permissive layout flags prevent structural evolution from discarding elements
    if (auto err = glz::read_file_json(m_current_config, filepath, buffer)) {
        // Automatically rewrite updated binary defaults on standard parsing failures
        [[maybe_unused]] auto write_err = glz::write_file_json(
            m_current_config, filepath, buffer
        );
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
    for (const auto& observer : m_observers) {
        observer(m_current_config);
    }
}

auto ConfigManager::register_observer(observer_callback callback) noexcept -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.push_back(std::move(callback));
    m_observers.back()(m_current_config);
}

} // namespace malama::config

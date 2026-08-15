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

#include <spdlog/spdlog.h>

namespace malama::config {

auto ConfigManager::get_instance() noexcept -> ConfigManager & {
    static ConfigManager instance;
    return instance;
}

auto ConfigManager::load_config(const std::string &filepath) noexcept -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string buffer;

    if (auto err = glz::read_file_json(m_current_config, filepath, buffer)) {
        spdlog::warn("Failed to parse config file: {}. Auto-writing default configuration.",
                     filepath);
        [[maybe_unused]] auto write_err = glz::write_file_json(m_current_config, filepath, buffer);
    } else {
        spdlog::info("Successfully loaded application configuration from {}", filepath);
    }
}

auto ConfigManager::save_config(const std::string &filepath) noexcept -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string buffer;

    if (auto err = glz::write_file_json(m_current_config, filepath, buffer)) {
        spdlog::error("Failed to persist application configuration to disk: {}", filepath);
    } else {
        spdlog::info("Application configuration successfully written to {}", filepath);
    }
}

auto ConfigManager::get_config() const noexcept -> AppConfig {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_current_config;
}

auto ConfigManager::update_config(const AppConfig &new_config, const std::string &filepath) noexcept
    -> void {
    std::vector<observer_callback> observers_snapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_current_config = new_config;

        std::string buffer;
        [[maybe_unused]] auto write_err = glz::write_file_json(m_current_config, filepath, buffer);
        observers_snapshot = m_observers;
    }

    for (const auto &observer : observers_snapshot) {
        if (observer) {
            observer(new_config);
        }
    }
}

auto ConfigManager::register_observer(observer_callback callback) noexcept -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.push_back(callback);

    if (callback) {
        callback(m_current_config);
    }
}

}  // namespace malama::config

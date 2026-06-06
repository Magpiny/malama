// /////////////////////////////////////////////////////////////////////////////
// Name:        src/core/config_manager.cpp
// Purpose:     Exception-free data processing logic for application settings
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-06
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "core/config_manager.hpp"
#include <spdlog/spdlog.h>

namespace malama::core {

ConfigManager::ConfigManager(std::string config_path) noexcept 
    : m_target_filepath(std::move(config_path)) {}

auto ConfigManager::LoadSettings() noexcept 
    -> std::expected<common::AppSettings, common::ConfigError> {
    spdlog::debug("Accessing configuration asset path: {}", m_target_filepath);
    
    // For now, we return a cleanly initialized default object stack.
    // In later updates, Glaze will populate this safely without exceptions.
    return m_cached_state;
}

auto ConfigManager::SaveSettings(const common::AppSettings &settings) noexcept 
    -> std::expected<void, common::ConfigError> {
    m_cached_state = settings;
    spdlog::info("Application operational constraints updated cleanly.");
    return {};
}

} // namespace malama::core

// /////////////////////////////////////////////////////////////////////////////
// Name:        src/core/config_manager.hpp
// Purpose:     Concrete exception-free configuration manager implementation
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-06
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include "core/i_config_manager.hpp"

namespace malama::core {

class ConfigManager final : public IConfigManager {
public:
    explicit ConfigManager(std::string config_path) noexcept;
    ~ConfigManager() override = default;

    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;
    ConfigManager(ConfigManager &&) noexcept = default;
    ConfigManager &operator=(ConfigManager &&) noexcept = default;

    [[nodiscard]] auto LoadSettings() noexcept 
        -> std::expected<common::AppSettings, common::ConfigError> override;

    [[nodiscard]] auto SaveSettings(const common::AppSettings &settings) noexcept 
        -> std::expected<void, common::ConfigError> override;

private:
    std::string m_target_filepath;
    common::AppSettings m_cached_state;
};

} // namespace malama::core

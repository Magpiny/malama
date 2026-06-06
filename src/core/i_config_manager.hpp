// /////////////////////////////////////////////////////////////////////////////
// Name:        src/core/i_config_manager.hpp
// Purpose:     Pure abstract boundary interface for application configurations
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-06
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include "common/types.hpp"
#include <expected>

namespace malama::core {

class IConfigManager {
public:
    virtual ~IConfigManager() = default;

    [[nodiscard]] virtual auto LoadSettings() noexcept 
        -> std::expected<common::AppSettings, common::ConfigError> = 0;

    [[nodiscard]] virtual auto SaveSettings(const common::AppSettings &settings) noexcept 
        -> std::expected<void, common::ConfigError> = 0;
};

} // namespace malama::core

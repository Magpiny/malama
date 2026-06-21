// /////////////////////////////////////////////////////////////////////////////
// Name:        include/config/config_manager.hpp
// Purpose:     malama app config manager
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <glaze/glaze.hpp>

#include "../src/common/constants.hpp"

namespace malama::config {

struct EngineConfig {
    std::string m_host{"127.0.0.1"};
    std::string m_port{"11434"};
    std::string m_active_model{"qwen2.5-coder:7b"};
};

struct AppearanceConfig {
    std::string m_theme_name{"dark_maroon"};
    std::string m_bg_color{"#2d0309"};
    std::string m_surface_color{"#420912"};
    std::string m_text_primary{"#f5f5f7"};
    std::string m_text_accent{"#c4929a"};
    std::string m_code_bg{"#1a0105"};
    std::string m_code_keyword{"#ff7b72"}; // Pink/Red for keywords
    std::string m_code_string{"#a5d6ff"};  // Blue for strings
    std::string m_code_type{"#d2a8ff"};    // Purple for types/classes
    std::string m_code_comment{"#8b949e"}; // Grey for comments
    std::string m_code_line_num{"#666666"};
};

struct InteractionConfig {
    int m_typewriter_delay_ms{constants::typewriter_delay_timer_ms };
};

struct AppConfig {
    EngineConfig m_engine;
    AppearanceConfig m_appearance;
    InteractionConfig m_interaction;
};

class ConfigManager {
public:
    static auto get_instance() noexcept -> ConfigManager&;

    auto load_config(const std::string& filepath = "malama_config.json") noexcept -> void;
    auto save_config(const std::string& filepath = "malama_config.json") noexcept -> void;
    
    [[nodiscard]] auto get_config() const noexcept -> const AppConfig&;
    auto update_config(const AppConfig& new_config) noexcept -> void;

    // Observer Pattern: Register callbacks for when config changes
    using observer_callback = std::function<void(const AppConfig&)>;
    auto register_observer(observer_callback callback) noexcept -> void;

private:
    ConfigManager() = default;
    AppConfig m_current_config;
    std::vector<observer_callback> m_observers;
    std::mutex m_mutex;
};

} // namespace malama::config

// Glaze Meta mappings for zero-overhead JSON serialization
template <>
struct glz::meta<malama::config::EngineConfig> {
    using T = malama::config::EngineConfig;
    static constexpr auto value = object(
        "host", &T::m_host,
        "port", &T::m_port,
        "active_model", &T::m_active_model
    );
};

template <>
struct glz::meta<malama::config::AppearanceConfig> {
    using T = malama::config::AppearanceConfig;
    static constexpr auto value = object(
        "theme_name", &T::m_theme_name,
        "bg_color", &T::m_bg_color,
        "surface_color", &T::m_surface_color,
        "text_primary", &T::m_text_primary,
        "text_accent", &T::m_text_accent,
        "code_bg", &T::m_code_bg,
        "code_keyword", &T::m_code_keyword,
        "code_string", &T::m_code_string,
        "code_type", &T::m_code_type,
        "code_comment", &T::m_code_comment,
        "code_line_num", &T::m_code_line_num
    );
};

template <>
struct glz::meta<malama::config::InteractionConfig> {
    using T = malama::config::InteractionConfig;
    static constexpr auto value = object(
        "typewriter_delay_ms", &T::m_typewriter_delay_ms
    );
};

template <>
struct glz::meta<malama::config::AppConfig> {
    using T = malama::config::AppConfig;
    static constexpr auto value = object(
        "engine", &T::m_engine,
        "appearance", &T::m_appearance,
        "interaction", &T::m_interaction
    );
};

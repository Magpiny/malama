// /////////////////////////////////////////////////////////////////////////////
// Name:        include/config/config_manager.hpp
// Purpose:     Thread-safe configuration reflection architecture
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

#include <functional>
#include <glaze/glaze.hpp>
#include <mutex>
#include <string>
#include <vector>

#include "common/constants.hpp"

namespace malama::config {

struct EngineConfig final {
    std::string m_host{constants::default_ollama_host};
    std::string m_port{constants::default_ollama_port};
    std::string m_active_model = constants::fallback_model_name;
    bool m_thinking_enabled{false};
};

struct AppearanceConfig final {
    std::string m_theme_name{"malama_default"};
    std::string m_bg_color{constants::chatpanel_bg_color};
    std::string m_surface_color{constants::chatinput_bg_color};
    std::string m_text_primary{constants::chatinput_text_color};
    std::string m_text_accent{"#c4929a"};
    std::string m_code_bg{constants::codeblock_bg_color};
    std::string m_code_keyword{"#ff7b72"};
    std::string m_code_string{"#a5d6ff"};
    std::string m_code_type{"#d2a8ff"};
    std::string m_code_comment{"#8b949e"};
    std::string m_code_line_num{"#666666"};

    // FIXED: Added missing tracking variables for v0.2.6 milestones
    std::string m_sidebar_bg{constants::sidebar_bg_color};
    std::string m_sidebar_text{constants::body_text_color};
    int m_font_size{constants::default_font_size};
    std::string m_font_family{"Sans"};
};

struct InteractionConfig final {
    int m_typewriter_delay_ms{constants::typewriter_delay_timer_ms};
};

struct AppConfig final {
    EngineConfig m_engine;
    AppearanceConfig m_appearance;
    InteractionConfig m_interaction;
};

class ConfigManager final {
   public:
    static auto get_instance() noexcept -> ConfigManager &;

    ~ConfigManager() = default;

    ConfigManager(const ConfigManager &) = delete;
    auto operator=(const ConfigManager &) -> ConfigManager & = delete;
    ConfigManager(ConfigManager &&) noexcept = delete;
    auto operator=(ConfigManager &&) noexcept -> ConfigManager & = delete;

    auto load_config(const std::string &filepath = "malama_config.json") noexcept -> void;
    auto save_config(const std::string &filepath = "malama_config.json") noexcept -> void;

    [[nodiscard]] auto get_config() const noexcept -> AppConfig;
    auto update_config(const AppConfig &new_config) noexcept -> void;

    using observer_callback = std::function<void(const AppConfig &)>;
    auto register_observer(observer_callback callback) noexcept -> void;

   private:
    ConfigManager() = default;

    AppConfig m_current_config;
    std::vector<observer_callback> m_observers;
    mutable std::mutex m_mutex;
};

}  // namespace malama::config

template<>
struct glz::meta<malama::config::EngineConfig> {
    using T = malama::config::EngineConfig;
    static constexpr auto value =
        object("host", &T::m_host, "port", &T::m_port, "active_model", &T::m_active_model,
               "thinking_enabled", &T::m_thinking_enabled);
};

template<>
struct glz::meta<malama::config::AppearanceConfig> {
    using T = malama::config::AppearanceConfig;
    static constexpr auto value = object(
        "theme_name", &T::m_theme_name, "bg_color", &T::m_bg_color, "surface_color",
        &T::m_surface_color, "text_primary", &T::m_text_primary, "text_accent", &T::m_text_accent,
        "code_bg", &T::m_code_bg, "code_keyword", &T::m_code_keyword, "code_string",
        &T::m_code_string, "code_type", &T::m_code_type, "code_comment", &T::m_code_comment,
        "code_line_num", &T::m_code_line_num, "sidebar_bg", &T::m_sidebar_bg, "sidebar_text",
        &T::m_sidebar_text, "font_size", &T::m_font_size, "font_family", &T::m_font_family);
};

template<>
struct glz::meta<malama::config::InteractionConfig> {
    using T = malama::config::InteractionConfig;
    static constexpr auto value = object("typewriter_delay_ms", &T::m_typewriter_delay_ms);
};

template<>
struct glz::meta<malama::config::AppConfig> {
    using T = malama::config::AppConfig;
    static constexpr auto value = object("engine", &T::m_engine, "appearance", &T::m_appearance,
                                         "interaction", &T::m_interaction);
};

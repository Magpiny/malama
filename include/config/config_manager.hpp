// /////////////////////////////////////////////////////////////////////////////
// Name:        include/config/config_manager.hpp
// Purpose:     Thread-safe configuration reflection architecture
// Author:      Wanjare <wanjare@magpiny.dev>
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

/**
 * @brief Ollama endpoint settings and model defaults.
 */
struct EngineConfig final {
    std::string m_host{constants::default_ollama_host};
    std::string m_port{constants::default_ollama_port};
    std::string m_active_model = constants::fallback_model_name;
    bool m_thinking_enabled{false};
};

/**
 * @brief UI styling rules, colors, and typography settings.
 */
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

/**
 * @brief Interactive delay metrics and UI timing options.
 */
struct InteractionConfig final {
    int m_typewriter_delay_ms{constants::typewriter_delay_timer_ms};
};

/**
 * @brief Consolidated root application configuration state structure.
 */
struct AppConfig final {
    EngineConfig m_engine{};
    AppearanceConfig m_appearance{};
    InteractionConfig m_interaction{};
};

/**
 * @brief Singleton configuration controller handling file I/O and UI observers.
 */
class ConfigManager final {
   public:
    /**
     * @brief Access the global singleton ConfigManager instance.
     * @return Reference to the thread-safe static ConfigManager instance.
     */
    static auto get_instance() noexcept -> ConfigManager &;

    ~ConfigManager() = default;

    ConfigManager(const ConfigManager &) = delete;
    auto operator=(const ConfigManager &) -> ConfigManager & = delete;
    ConfigManager(ConfigManager &&) noexcept = delete;
    auto operator=(ConfigManager &&) noexcept -> ConfigManager & = delete;

    /**
     * @brief Load configuration from JSON disk file into memory.
     * @param filepath Path to JSON configuration file.
     */
    auto load_config(const std::string &filepath = "malama_config.json") noexcept -> void;

    /**
     * @brief Save active configuration state to JSON file on disk.
     * @param filepath Path to destination JSON configuration file.
     */
    auto save_config(const std::string &filepath = "malama_config.json") noexcept -> void;

    /**
     * @brief Thread-safe getter for current AppConfig state.
     * @return Stack copy of active AppConfig state.
     */
    [[nodiscard]] auto get_config() const noexcept -> AppConfig;

    /**
     * @brief Update active AppConfig and flush changes to observers and disk.
     * @param new_config Updated application configuration object.
     * @param filepath Path to destination JSON configuration file.
     */
    auto update_config(const AppConfig &new_config,
                       const std::string &filepath = "malama_config.json") noexcept -> void;

    using observer_callback = std::function<void(const AppConfig &)>;

    /**
     * @brief Register UI observer callback triggered on configuration changes.
     * @param callback Function callback listening for AppConfig state updates.
     */
    auto register_observer(observer_callback callback) noexcept -> void;

   private:
    ConfigManager() = default;

    AppConfig m_current_config{};
    std::vector<observer_callback> m_observers{};
    mutable std::mutex m_mutex{};
};

}  // namespace malama::config

// Glaze Meta mappings for zero-overhead JSON serialization
template<>
struct glz::meta<malama::config::EngineConfig> {
    using Type = malama::config::EngineConfig;
    static constexpr auto value =
        glz::object("host", &Type::m_host, "port", &Type::m_port, "active_model",
                    &Type::m_active_model, "thinking_enabled", &Type::m_thinking_enabled);
};

template<>
struct glz::meta<malama::config::AppearanceConfig> {
    using Type = malama::config::AppearanceConfig;
    static constexpr auto value = glz::object(
        "theme_name", &Type::m_theme_name, "bg_color", &Type::m_bg_color, "surface_color",
        &Type::m_surface_color, "text_primary", &Type::m_text_primary, "text_accent",
        &Type::m_text_accent, "code_bg", &Type::m_code_bg, "code_keyword", &Type::m_code_keyword,
        "code_string", &Type::m_code_string, "code_type", &Type::m_code_type, "code_comment",
        &Type::m_code_comment, "code_line_num", &Type::m_code_line_num, "sidebar_bg",
        &Type::m_sidebar_bg, "sidebar_text", &Type::m_sidebar_text, "font_size", &Type::m_font_size,
        "font_family", &Type::m_font_family);
};

template<>
struct glz::meta<malama::config::InteractionConfig> {
    using Type = malama::config::InteractionConfig;
    static constexpr auto value = glz::object("typewriter_delay_ms", &Type::m_typewriter_delay_ms);
};

template<>
struct glz::meta<malama::config::AppConfig> {
    using Type = malama::config::AppConfig;
    static constexpr auto value =
        glz::object("engine", &Type::m_engine, "appearance", &Type::m_appearance, "interaction",
                    &Type::m_interaction);
};

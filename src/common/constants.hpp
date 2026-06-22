// /////////////////////////////////////////////////////////////////////////////
// Name:        src/common/constants.hpp
// Purpose:     Global compile-time named constants for malama
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-06
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <string_view>

namespace malama::constants {

// Default Window Sizing Metrics
inline constexpr int default_window_width = 1200;
inline constexpr int default_window_height = 800;

// Internal API & Network Target Anchors
inline constexpr std::string_view default_ollama_endpoint = "http://localhost:11434";
inline constexpr std::string_view default_ollama_host = "127.0.0.1";
inline constexpr std::string_view default_ollama_port = "11434";
inline constexpr std::string_view ollama_generate_path = "/api/generate";
inline constexpr std::string_view fallback_model_name = "qwen2.5-coder:7b";

// Memory & Processing Constraints
inline constexpr std::size_t absolute_max_buffer_bytes = 8192;
inline constexpr std::size_t default_context_window_limit = 4096;

// Protocol Handshake Definitions
inline constexpr unsigned http_version_1_1 = 11;

// Layout Workspace Sizing & Proportional Scaling Constants
inline constexpr int default_margin_padding = 8;
inline constexpr int zero_margin_padding = 0;
inline constexpr int layout_proportion_fixed = 0;
inline constexpr int layout_proportion_stretch = 1;
inline constexpr int sidebar_layout_weight = 1;
inline constexpr int chat_layout_weight = 3;
inline constexpr int input_area_height_pixels = 80;

// Draggable Splitter Window Geometric Constraints
inline constexpr int default_sash_position = 300;
inline constexpr int minimum_pane_size_pixels = 200;

// UI Theming Palette & Typography
inline constexpr std::string_view color_earth_brown = "#5C4033";
inline constexpr std::string_view color_dark_brown = "#3E2723";
//inline constexpr std::string_view color_smoke_white = "#F5F5F5";

// Dark Maroon UI Hex Theme Palette
inline constexpr std::string_view color_dark_maroon = "#2d0309";      // Primary panel background
inline constexpr std::string_view color_maroon_surface = "#420912";   // Input boxes and text containers
inline constexpr std::string_view color_maroon_element = "#5c101c";   // Buttons and interactive components
inline constexpr std::string_view color_smoke_white = "#f5f5f7";      // Body text color
inline constexpr std::string_view color_muted_rose = "#c4929a";       // Headers and system status labels
inline constexpr std::string_view color_code_background = "#1a0105";   //

// Button & Icon Styling Metrics
inline constexpr int icon_button_margin = 4;

// Asynchronous Network Timeout Thresholds
inline constexpr int NETWORK_RESOLVE_TIMEOUT_SEC = 15;
inline constexpr int NETWORK_CONNECT_TIMEOUT_SEC = 10;
inline constexpr int NETWORK_WRITE_TIMEOUT_SEC = 30;
inline constexpr int NETWORK_READ_TIMEOUT_SEC = 60;


inline constexpr int typewriter_delay_timer_ms = 5;
} // namespace malama::constants

// /////////////////////////////////////////////////////////////////////////////
// Name:        src/common/constants.hpp
// Purpose:     Global compile-time named constants for malama fallbacks
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-06
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <string_view>

namespace malama::constants {

// Default Window Sizing Metrics
inline constexpr int default_window_width = 1200;
inline constexpr int default_window_height = 800;
inline constexpr int default_font_size = 13;

// Internal API & Network Target Anchors (Strictly Used as Initial Configuration Fallbacks)
inline constexpr std::string_view default_ollama_endpoint = "http://localhost:11434";
inline constexpr std::string_view default_ollama_host = "127.0.0.1";
inline constexpr std::string_view default_ollama_port = "11434";
inline constexpr std::string_view ollama_generate_path = "/api/generate";
inline constexpr std::string_view ollama_chat_path = "/api/chat";  // Unified chat array route
inline constexpr std::string_view fallback_model_name = "ornith:latest";

// Memory & Processing Constraints
inline constexpr std::size_t absolute_max_buffer_bytes = 1024UZ * 1024UZ;
inline constexpr std::size_t default_context_window_limit = 4048;

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

// Dark Maroon UI Hex Theme Palette Mappings
inline constexpr std::string_view color_dark_maroon = "#2d0309";      // Primary panel background
inline constexpr std::string_view color_maroon_surface = "#420912";   // Input boxes and text panels
inline constexpr std::string_view color_maroon_element = "#5c101c";   // Buttons and controls
inline constexpr std::string_view color_smoke_white = "#f5f5f7";      // Body text color
inline constexpr std::string_view color_muted_rose = "#c4929a";       // Headers and status labels
inline constexpr std::string_view color_code_background = "#1a0105";  // Codeblock view terminal
inline constexpr std::string_view color_dark_brown = "#3E2723";       // sidebar color //
inline constexpr std::string_view color_dark_grey = "#282F31";        // sidebar color //

// Button & Icon Styling Metrics
inline constexpr int icon_button_margin = 4;

// Asynchronous Network Timeout Thresholds (Seconds)
inline constexpr int NETWORK_RESOLVE_TIMEOUT_SEC = 15;
inline constexpr int NETWORK_CONNECT_TIMEOUT_SEC = 10;
inline constexpr int NETWORK_WRITE_TIMEOUT_SEC = 30;
inline constexpr int NETWORK_READ_TIMEOUT_SEC = 60;

// Typewriter Rendering Parameters
inline constexpr int typewriter_delay_timer_ms = 5;

}  // namespace malama::constants

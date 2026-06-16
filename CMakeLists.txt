# /////////////////////////////////////////////////////////////////////////////
# Name:        CMakeLists.txt
# Purpose:     Root modern target-based compilation build for malama
# Author:      Wanjare <wanjare@magpiny.dev>
# Created:     2026-06-10
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     Apache-2.0
# /////////////////////////////////////////////////////////////////////////////

cmake_minimum_required(VERSION 3.28)
project(malama CXX)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(SPDLOG_USE_STD_FORMAT)

add_executable(malama
    src/main.cpp
    src/core/config_manager.cpp
    src/network/ollama_client.cpp
    src/network/stream_worker.cpp
    src/ui/main_frame.cpp
    src/ui/sidebar_panel.cpp
    src/ui/chat_panel.cpp
    src/ui/settings_dialog.cpp
    src/config/config_manager.cpp
    src/engine/markdown/pipeline.cpp
    src/engine/markdown/syntax_registry.cpp
)

# -----------------------------------------------------------------------------
# 3. SET INCLUDE DIRECTORIES
# This tells the compiler where to look when you say #include "config/..."
# -----------------------------------------------------------------------------
target_include_directories(malama PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_features(malama PUBLIC cxx_std_23)

target_include_directories(malama
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_compile_options(malama PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -O3
    -march=native
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
)

# Extract native UI components
find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base html)
include(${wxWidgets_USE_FILE})
target_link_libraries(malama PRIVATE ${wxWidgets_LIBRARIES})

# Explicitly require the Boost framework to guarantee hermetic environment resolution
find_package(Boost REQUIRED)
target_link_libraries(malama PRIVATE Boost::boost)

include(FetchContent)

# Provision spdlog framework dependencies (isolated inside build tree)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.17.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(malama PRIVATE spdlog::spdlog)

# Provision Glaze JSON parser dependency (isolated inside build tree)
FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG        v7.8.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glaze)

# Fixed: Explicitly link against the canonical namespaced header interface target
target_link_libraries(malama PRIVATE glaze::glaze)

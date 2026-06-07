# /////////////////////////////////////////////////////////////////////////////
# Name:        CMakeLists.txt
# Purpose:     Root modern target-based compilation build for malama
# Author:      Wanjare <wanjare@magpiny.dev>
# Created:     2026-06-06
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     Apache-2.0
# /////////////////////////////////////////////////////////////////////////////

cmake_minimum_required(VERSION 4.2)
project(malama CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(SPDLOG_USE_STD_FORMAT)

# Add all source translation units explicitly
add_executable(malama
    src/main.cpp
    src/core/config_manager.cpp
    src/network/ollama_client.cpp
    src/network/stream_worker.cpp
    src/ui/main_frame.cpp
)

target_compile_features(malama PUBLIC cxx_std_23)

target_include_directories(malama
    PRIVATE ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/libs
)

target_compile_options(malama PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -O3
    -march=native
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
)

find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})
target_link_libraries(malama PRIVATE ${wxWidgets_LIBRARIES})

find_package(Boost REQUIRED)
target_link_libraries(malama PRIVATE Boost::boost)

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/libs/spdlog
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(malama PRIVATE spdlog::spdlog)

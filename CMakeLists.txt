# /////////////////////////////////////////////////////////////////////////////
# Name:        CMakeLists.txt
# Purpose:     Root modern target-based compilation build for malama
# Author:      Wanjare S. <samuelwanjare@protonmail.com>
# Created:     2026-06-10
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     Apache-2.0
# /////////////////////////////////////////////////////////////////////////////

cmake_minimum_required(VERSION 3.28)
project(malama VERSION 0.2.7 LANGUAGES CXX)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Hardened Compiler Diagnostic and Optimization Configurations
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        -Wextra-semi -Wold-style-cast -Wnon-virtual-dtor -Wunused
        -O3 -march=native -fno-plt -fstack-protector-strong
    )
endif()

# -----------------------------------------------------------------------------
# DEPENDENCY RESOLUTION SECTION
# -----------------------------------------------------------------------------

# Find core multi-threading library primitives
find_package(Threads REQUIRED)

# Discover local installation footprint of wxWidgets 3.2+
find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base html)
include(${wxWidgets_USE_FILE})

# FIXED: Removed 'cobalt' from components to prevent missing binary errors
find_package(Boost REQUIRED COMPONENTS regex iostreams cobalt context)
find_package(pugixml REQUIRED)
find_package(LibArchive REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(SQLite3 REQUIRED)

# Extract native Poppler configuration layouts via PkgConfig
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED poppler-cpp)

# Provision third-party frameworks from remote sources
include(FetchContent)

set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.17.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG        v7.8.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glaze)

# -----------------------------------------------------------------------------
# TARGET CREATION AND LINKAGE
# -----------------------------------------------------------------------------

add_executable(malama
    src/main.cpp
    src/core/config_manager.cpp
    src/network/ollama_client.cpp
    src/network/stream_worker.cpp
    src/ui/main_frame.cpp
    src/ui/sidebar_panel.cpp
    src/ui/chat_panel.cpp
    src/ui/settings_dialog.cpp
    src/ui/error_banner.cpp
    src/config/config_manager.cpp
    src/engine/markdown/pipeline.cpp
    src/engine/markdown/syntax_registry.cpp
    src/engine/storage/history_manager.cpp
    src/engine/storage/attachment_manager.cpp
    src/engine/storage/parsers/archive_reader.cpp
    src/engine/storage/parsers/plain_text_parser.cpp
    src/engine/storage/parsers/xml_parser.cpp
    src/engine/storage/parsers/docx_parser.cpp
    src/engine/storage/parsers/odt_parser.cpp
    src/engine/storage/parsers/epub_parser.cpp
    src/engine/storage/parsers/pdf_parser.cpp
    src/engine/storage/parsers/xlsx_parser.cpp
    src/engine/storage/parsers/ods_parser.cpp
    src/engine/storage/parsers/image_parser.cpp
    src/engine/storage/parsers/parser_factory.cpp
)

target_compile_definitions(malama PRIVATE SPDLOG_USE_STD_FORMAT)
target_compile_features(malama PUBLIC cxx_std_23)

target_include_directories(malama PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${LibArchive_INCLUDE_DIRS}
    ${POPPLER_CPP_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIR}
    ${PNG_INCLUDE_DIRS}
)

target_link_libraries(malama PRIVATE
    Threads::Threads
    ${wxWidgets_LIBRARIES}
    Boost::boost
    Boost::regex
    Boost::cobalt
    Boost::iostreams
    Boost::context
    pugixml
    SQLite3::SQLite3
    spdlog::spdlog
    glaze::glaze
    ${LibArchive_LIBRARIES}
    ${POPPLER_CPP_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${PNG_LIBRARIES}
)

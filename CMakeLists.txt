#/////////////////////////////////////////////////////////////////////////////
#Name : CMakeLists.txt
#Purpose : Root build orchestration with fully resolved symbol linking
#Author : Wanjare S.< samuelwanjare @protonmail.com>
#Created : 2026 - 07 - 16
#Copyright : (c)2026 Magpiny.All rights reserved.
#Licence : GPL - 3.0 - or -later
#/////////////////////////////////////////////////////////////////////////////

cmake_minimum_required(VERSION 3.28)
project(malama VERSION 0.3.0 LANGUAGES CXX)

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
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wextra-semi
    -Wold-style-cast
    -Wnon-virtual-dtor
    -Wunused
    -O3
    -march=native
    -fno-plt
    -fstack-protector-strong
  )
endif()

# Append custom script path matrices into global index arrays
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ResolveDependency)

#------------------------------------------------------------------------------
# Interactive System Dependency Checking Matrix Blocks
#------------------------------------------------------------------------------

resolve_dependency(
  TARGET_NAME "spdlog"
  FIND_NAME "spdlog"
  REASON "Required for colorized asynchronous console application logs."
  PACMAN_NAME "spdlog"
  APT_NAME "libspdlog-dev"
  DNF_NAME "spdlog-devel"
)

resolve_dependency(
  TARGET_NAME "pugixml"
  FIND_NAME "pugixml"
  REASON "Required to perform light XPath node traversals inside files."
  PACMAN_NAME "pugixml"
  APT_NAME "libpugixml-dev"
  DNF_NAME "pugixml-devel"
)

resolve_dependency(
  TARGET_NAME "LibArchive"
  FIND_NAME "LibArchive"
  REASON "Provides memory-safe decompression streams preventing zip threats."
  PACMAN_NAME "libarchive"
  APT_NAME "libarchive-dev"
  DNF_NAME "libarchive-devel"
)

include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v7.8.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)

#------------------------------------------------------------------------------
# Core System Verification Fallbacks
#------------------------------------------------------------------------------
find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base html adv)
find_package(SQLite3 REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)

# FIXED: Explicitly map and resolve compiled binary symbols for Boost Cobalt coroutines
find_package(Boost REQUIRED COMPONENTS cobalt)

find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)

#------------------------------------------------------------------------------
# Interactive System Dependency Checking Matrix Blocks
#------------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED IMPORTED_TARGET glib-2.0)

#------------------------------------------------------------------------------
# Target Specification: malama_core Static Library Subsystem
#------------------------------------------------------------------------------
add_library(
  malama_core STATIC
  src/config/config_manager.cpp
  src/core/install_validator.cpp
  src/engine/markdown/pipeline.cpp
  src/engine/markdown/syntax_registry.cpp # FIXED: Added missing domain component
  src/engine/storage/attachment_manager.cpp
  src/engine/storage/history_manager.cpp
  src/engine/storage/parsers/archive_reader.cpp
  src/engine/storage/parsers/docx_parser.cpp
  src/engine/storage/parsers/epub_parser.cpp
  src/engine/storage/parsers/image_parser.cpp
  src/engine/storage/parsers/ods_parser.cpp
  src/engine/storage/parsers/odt_parser.cpp
  src/engine/storage/parsers/parser_factory.cpp
  src/engine/storage/parsers/pdf_parser.cpp
  src/engine/storage/parsers/plain_text_parser.cpp
  src/engine/storage/parsers/xlsx_parser.cpp
  src/engine/storage/parsers/xml_parser.cpp
  src/network/ollama_client.cpp
  src/network/stream_worker.cpp
  src/engine/export/export_engine.cpp
  src/engine/token/token_estimator.cpp
)

target_include_directories(
  malama_core
  PUBLIC
    include
    src
    libs
)

target_link_libraries(
  malama_core
  PUBLIC
    SQLite3::SQLite3
    glaze::glaze
    pugixml
    PNG::PNG
    JPEG::JPEG
    PkgConfig::POPPLER_CPP
    spdlog::spdlog
    Boost::cobalt
    LibArchive::LibArchive
    ${wxWidgets_LIBRARIES}
)

#------------------------------------------------------------------------------
# Target Specification: malama Desktop Graphical User Interface
#------------------------------------------------------------------------------
add_executable(
  malama
  src/main.cpp
  src/ui/chat_panel.cpp
  src/ui/context_bar.cpp
  src/ui/error_banner.cpp
  src/ui/main_frame.cpp
  src/ui/session_params_dlg.cpp
  src/ui/settings_dialog.cpp
  src/ui/sidebar_panel.cpp
  src/ui/token_budget_bar.cpp
)

include(${wxWidgets_USE_FILE})

target_link_libraries(
  malama
  PRIVATE
    malama_core
    ${wxWidgets_LIBRARIES}
    PkgConfig::GLIB2
)

#------------------------------------------------------------------------------
# Adding icon to user to mainframe
#------------------------------------------------------------------------------
# Add post-build command to automatically sync runtime assets to the build directory
add_custom_command(
  TARGET malama
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/assets"
          "$<TARGET_FILE_DIR:malama>/assets"
  COMMENT "Copying runtime asset files (malama.jpg) to build directory..."
)

#------------------------------------------------------------------------------
# Conditional Testing Engine Loading
#------------------------------------------------------------------------------
if(BUILD_TESTING OR CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  add_subdirectory(tests)
endif()

#------------------------------------------------------------------------------
# Desktop Entry & Icon Asset Configurations
#------------------------------------------------------------------------------
include(GNUInstallDirs)

# Configure malama.desktop in build directory for development/testing
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/assets/malama.desktop.in"
  "${CMAKE_BINARY_DIR}/malama.desktop"
  @ONLY
)

# Install rules for production deployment (make install)
install(
  TARGETS malama
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  FILES "${CMAKE_BINARY_DIR}/malama.desktop"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/malama.png")
  install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/malama.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
  )
endif()

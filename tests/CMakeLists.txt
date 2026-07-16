# // ///////////////////////////////////////////////////////////////////////////
# Name:        tests/CMakeLists.txt
# Purpose:     Modular test execution harness relying on Catch2 v3 automation
# Author:      Wanjare S. <samuelwanjare@protonmail.com>
# Created:     2026-07-15
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     GPL-3.0-or-later
# // ///////////////////////////////////////////////////////////////////////////

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
    SYSTEM
)

FetchContent_MakeAvailable(Catch2)

# FIXED: Explicitly append Catch2's custom cmake module path to the search registry
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

enable_testing()

# FIXED: Catch2 v3 provides its discovery macros inside 'Catch.cmake' rather than 'Catch2'
include(Catch)

# ---------------------------------------------------------------------------
# Target Definition: malama_tests (Isolated Execution Runner Workspace)
# ---------------------------------------------------------------------------
add_executable(malama_tests
    main_test.cpp
    unit/test_base64.cpp
    unit/test_mime_detector.cpp
    unit/test_attachment_bounds.cpp
    unit/test_context_enclosure.cpp
)

target_compile_features(malama_tests PRIVATE cxx_std_23)

target_compile_options(malama_tests PRIVATE
    -Wall -Wextra -Wno-c2y-extensions -Wpedantic
    -fstack-protector-strong
)

target_link_libraries(malama_tests PRIVATE
    malama_core
    Catch2::Catch2WithMain
)

catch_discover_tests(malama_tests)

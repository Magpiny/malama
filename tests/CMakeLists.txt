# /////////////////////////////////////////////////////////////////////////////
# Name:        tests/CMakeLists.txt
# Purpose:     Unit test build configuration for malama — GTest via FetchContent
# Author:      Wanjare <wanjare@magpiny.dev>
# Created:     2026-06-08
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     Apache-2.0
# /////////////////////////////////////////////////////////////////////////////

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/libs/googletest
)
# Prevent GTest from overriding our compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# ---------------------------------------------------------------------------
# test_main_init — tests for MalamaApp::OnInit allocation guards and constants
# Does NOT require wxWidgets; tests pure C++ logic and constant values only.
# ---------------------------------------------------------------------------
add_executable(test_main_init
    ${CMAKE_CURRENT_SOURCE_DIR}/main_test.cpp
)

target_include_directories(test_main_init PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libs
)

target_compile_features(test_main_init PUBLIC cxx_std_23)

target_compile_options(test_main_init PRIVATE
    -Wall -Wextra -Wpedantic
    -fstack-protector-strong
)

target_link_libraries(test_main_init PRIVATE
    GTest::gtest
    GTest::gtest_main
    spdlog::spdlog
)

gtest_discover_tests(test_main_init)

# ---------------------------------------------------------------------------
# test_network_ui — tests for EVT_MALAMA_TOKEN event type and token handling
# Requires wxWidgets 3.2 (same version enforced by the PR diff in the root
# CMakeLists.txt).  If wxWidgets is absent the target is skipped gracefully.
# ---------------------------------------------------------------------------
find_package(wxWidgets 3.2 QUIET COMPONENTS core base)

if(wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})

    add_executable(test_network_ui
        ${CMAKE_CURRENT_SOURCE_DIR}/test_network_ui.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/main_frame.cpp
    )

    target_include_directories(test_network_ui PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/libs
    )

    target_compile_features(test_network_ui PUBLIC cxx_std_23)

    target_compile_definitions(test_network_ui PRIVATE SPDLOG_USE_STD_FORMAT)

    target_compile_options(test_network_ui PRIVATE
        -Wall -Wextra -Wpedantic
        -fstack-protector-strong
    )

    target_link_libraries(test_network_ui PRIVATE
        GTest::gtest
        ${wxWidgets_LIBRARIES}
        spdlog::spdlog
    )

    gtest_discover_tests(test_network_ui)
else()
    message(STATUS
        "wxWidgets 3.2 not found — skipping test_network_ui build target")
endif()

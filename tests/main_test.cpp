// /////////////////////////////////////////////////////////////////////////////
// Name:        tests/main_test.cpp
// Purpose:     Catch2 test runner session initializer with colored spdlog hooks
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-16
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_session.hpp>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[]) {
    // Inject the human-friendly colored logging layout globally
    spdlog::set_pattern("^[%T] [ℹ️ INFO] [%s:%#]%$ => %v");
    spdlog::info("Initializing malama_tests offline execution suite.");

    const int test_session_result = Catch::Session().run(argc, argv);

    if (test_session_result == 0) {
        spdlog::info("All offline logic validation vectors passed cleanly.");
    } else {
        spdlog::set_pattern("^[%T] [❌ ERRO] [%s:%#]%$ => %v");
        spdlog::error("Regression detected! Test exit code: {}", test_session_result);
    }

    return test_session_result;
}

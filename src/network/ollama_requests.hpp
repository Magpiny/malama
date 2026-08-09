// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_requests.hpp
// Purpose:     Glaze JSON schema definitions for outbound Ollama API POSTs
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <glaze/glaze.hpp>
#include <string>

namespace malama::network {

struct OllamaGenerateRequest final {
    std::string model{};
    std::string prompt{};
    bool stream{true};

    struct glaze {
        using T = OllamaGenerateRequest;
        static constexpr auto value =
            glz::object("model", &T::model, "prompt", &T::prompt, "stream", &T::stream);
    };
};

}  // namespace malama::network
// Name:        tests/stress/test_stream_performance.cpp
// Purpose:     Async stream parsing and token throughput performance vector
// Author:      Wanjare S.
// Created:     2026-08-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <format>
#include <numeric>
#include <span>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

#include "network/ollama_client.hpp"

namespace malama::tests {

namespace {

enum class BenchmarkMode { Sequential, ConcurrentStreams };

struct PerformanceMetrics {
    std::size_t m_total_tokens{0};
    double m_elapsed_seconds{0.0};
    double m_tokens_per_second{0.0};
    std::chrono::microseconds m_time_to_first_token{0};
};

[[nodiscard]] auto generate_mock_stream_payloads(std::size_t token_count)
    -> std::vector<std::string> {
    std::vector<std::string> payloads;
    payloads.reserve(token_count);

    for (std::size_t idx = 0; idx < token_count; ++idx) {
        payloads.push_back(
            std::format("{{\"model\":\"qwen2.5-coder\",\"response\":\"tok_{}\",\"done\":{}}}", idx,
                        (idx + 1 == token_count) ? "true" : "false"));
    }
    return payloads;
}

[[nodiscard]] auto evaluate_stream_throughput(std::span<const std::string> payloads,
                                              BenchmarkMode mode) -> PerformanceMetrics {
    PerformanceMetrics metrics;
    metrics.m_total_tokens = payloads.size();

    const auto start_time = std::chrono::steady_clock::now();
    bool first_token_captured = false;

    if (mode == BenchmarkMode::Sequential) {
        for (const auto &chunk : payloads) {
            auto parsed = OllamaClient::parse_chunk(chunk);
            if (!first_token_captured && parsed.has_value()) {
                const auto now = std::chrono::steady_clock::now();
                metrics.m_time_to_first_token =
                    std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);
                first_token_captured = true;
            }
        }
    }

    const auto end_time = std::chrono::steady_clock::now();
    const std::chrono::duration<double> duration = end_time - start_time;

    metrics.m_elapsed_seconds = duration.count();
    if (metrics.m_elapsed_seconds > 0.0) {
        metrics.m_tokens_per_second =
            static_cast<double>(metrics.m_total_tokens) / metrics.m_elapsed_seconds;
    }

    return metrics;
}

}  // namespace

TEST_CASE("Async Stream Large Token Processing Baseline", "[stress][performance]") {
    constexpr std::size_t k_large_token_count = 50'000;
    const auto stream_payloads = generate_mock_stream_payloads(k_large_token_count);

    REQUIRE(stream_payloads.size() == k_large_token_count);

    SECTION("Execute high-throughput streaming evaluation vector") {
        spdlog::info("Simulating stream ingestion for {} tokens...", k_large_token_count);

        const auto metrics = evaluate_stream_throughput(stream_payloads, BenchmarkMode::Sequential);

        spdlog::info(
            "Stream perf metrics: total={}, elapsed={:.4f}s, speed={:.2f} tok/s, TTFT={}us",
            metrics.m_total_tokens, metrics.m_elapsed_seconds, metrics.m_tokens_per_second,
            metrics.m_time_to_first_token.count());

        REQUIRE(metrics.m_tokens_per_second > 10'000.0);
        REQUIRE(metrics.m_time_to_first_token.count() < 500);
    }

    SECTION("Catch2 benchmark fixture for chunk parsing overhead") {
        BENCHMARK("JSON chunk decoding per token") {
            return evaluate_stream_throughput(stream_payloads, BenchmarkMode::Sequential);
        };
    }
}

}  // namespace malama::tests

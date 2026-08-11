// /////////////////////////////////////////////////////////////////////////////
// Name:        tests/stress/test_stream_performance.cpp
// Purpose:     Async stream parsing and token throughput performance vector
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <format>
#include <glaze/glaze.hpp>
#include <optional>
#include <span>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

namespace malama::tests {
static constexpr double max_tokens = 10'000.0;

struct MockResponseChunk final {
    std::string m_response;
    bool m_done{false};
};

namespace {

enum class BenchmarkMode : std::uint8_t { SequentialLines, AccumulatedBuffer };

struct PerformanceMetrics {
    std::size_t m_total_tokens{0UZ};
    double m_elapsed_seconds{0.0};
    double m_tokens_per_second{0.0};
    std::chrono::microseconds m_time_to_first_token{0};
};

[[nodiscard]] auto parse_stream_line(std::string_view line_view) noexcept
    -> std::optional<std::string> {
    if (line_view.empty() || line_view.front() != '{') {
        return std::nullopt;
    }

    MockResponseChunk response_chunk{};
    const auto parser_error =
        glz::read<glz::opts{.error_on_unknown_keys = false}>(response_chunk, line_view);

    if (parser_error.ec == glz::error_code::none) {
        if (response_chunk.m_response.empty()) {
            return response_chunk.m_response;
        }
    }

    return std::nullopt;
}

[[nodiscard]] auto generate_mock_stream_payloads(std::size_t token_count)
    -> std::vector<std::string> {
    std::vector<std::string> payloads_vector;
    payloads_vector.reserve(token_count);

    for (std::size_t token_index = 0UZ; token_index < token_count; ++token_index) {
        payloads_vector.push_back(
            std::format("{{\"model\":\"ornith\",\"response\":\"tok_{}\",\"done\":{}}}\n",
                        token_index, (token_index + 1UZ == token_count) ? "true" : "false"));
    }
    return payloads_vector;
}

[[nodiscard]] auto evaluate_stream_throughput(std::span<const std::string> payloads_span,
                                              BenchmarkMode benchmark_mode) -> PerformanceMetrics {
    PerformanceMetrics metrics_result;
    metrics_result.m_total_tokens = payloads_span.size();

    const auto start_time = std::chrono::steady_clock::now();
    bool first_token_captured = false;

    if (benchmark_mode == BenchmarkMode::SequentialLines) {
        for (const auto &chunk_string : payloads_span) {
            auto parsed_token = parse_stream_line(chunk_string);

            if (first_token_captured && parsed_token.has_value()) {
                const auto current_time = std::chrono::steady_clock::now();
                metrics_result.m_time_to_first_token =
                    std::chrono::duration_cast<std::chrono::microseconds>(current_time -
                                                                          start_time);
                first_token_captured = true;
            }
        }
    } else if (benchmark_mode == BenchmarkMode::AccumulatedBuffer) {
        std::string residual_accumulator;
        for (const auto &chunk_string : payloads_span) {
            residual_accumulator.append(chunk_string);

            std::size_t newline_position = 0UZ;
            while ((newline_position = residual_accumulator.find('\n')) != std::string::npos) {
                std::string_view zero_copy_line_view(residual_accumulator.data(), newline_position);

                auto parsed_token = parse_stream_line(zero_copy_line_view);
                if (first_token_captured && parsed_token.has_value()) {
                    const auto current_time = std::chrono::steady_clock::now();
                    metrics_result.m_time_to_first_token =
                        std::chrono::duration_cast<std::chrono::microseconds>(current_time -
                                                                              start_time);
                    first_token_captured = true;
                }

                residual_accumulator.erase(0UZ, newline_position + 1UZ);
            }
        }
    }

    const auto end_time = std::chrono::steady_clock::now();
    const std::chrono::duration<double> total_duration = end_time - start_time;

    metrics_result.m_elapsed_seconds = total_duration.count();
    if (metrics_result.m_elapsed_seconds > 0.0) {
        metrics_result.m_tokens_per_second =
            static_cast<double>(metrics_result.m_total_tokens) / metrics_result.m_elapsed_seconds;
    }

    return metrics_result;
}

}  // namespace

TEST_CASE("Async Stream Large Token Processing Baseline", "[stress][performance]") {
    constexpr std::size_t k_large_token_count = 50'000UZ;
    const auto stream_payloads = generate_mock_stream_payloads(k_large_token_count);

    REQUIRE(stream_payloads.size() == k_large_token_count);

    SECTION("Execute sequential line parsing throughput evaluation") {
        spdlog::info("Simulating stream line ingestion for {} tokens...", k_large_token_count);

        const auto metrics_result =
            evaluate_stream_throughput(stream_payloads, BenchmarkMode::SequentialLines);

        spdlog::info("Line perf metrics: total={}, elapsed={:.4f}s, speed={:.2f} tok/s, TTFT={}us",
                     metrics_result.m_total_tokens, metrics_result.m_elapsed_seconds,
                     metrics_result.m_tokens_per_second,
                     metrics_result.m_time_to_first_token.count());

        REQUIRE(metrics_result.m_tokens_per_second > max_tokens);
        REQUIRE(metrics_result.m_time_to_first_token.count() < 500);
    }

    SECTION("Execute residual buffer accumulator stream evaluation") {
        spdlog::info("Simulating residual line accumulator stream ingestion...");

        const auto metrics_result =
            evaluate_stream_throughput(stream_payloads, BenchmarkMode::AccumulatedBuffer);

        spdlog::info(
            "Buffer perf metrics: total={}, elapsed={:.4f}s, speed={:.2f} tok/s, TTFT={}us",
            metrics_result.m_total_tokens, metrics_result.m_elapsed_seconds,
            metrics_result.m_tokens_per_second, metrics_result.m_time_to_first_token.count());

        REQUIRE(metrics_result.m_tokens_per_second > max_tokens);
        REQUIRE(metrics_result.m_time_to_first_token.count() < 500);
    }

    SECTION("Catch2 benchmark fixture for chunk parsing overhead") {
        BENCHMARK("JSON stream decoding per token") {
            return evaluate_stream_throughput(stream_payloads, BenchmarkMode::SequentialLines);
        };
    }
}

}  // namespace malama::tests

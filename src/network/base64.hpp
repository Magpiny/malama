// /////////////////////////////////////////////////////////////////////////////
// Name:        include/network/base64.hpp
// Purpose:     High-speed stack-allocated string conversion utility for images
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <string_view>

namespace malama::network {

/// @brief Transforms raw binary data blocks into standard compliant Base64 string models.
/// @param input Read-only view pointing to binary asset memory.
/// @return Encoded ASCII string footprint representation.
[[nodiscard]] inline auto encode_base64(std::string_view input) -> std::string {
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);

    static constexpr char lookup[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::uint32_t bits = 0;
    std::int32_t bit_count = -6;

    for (const auto &character : input) {
        bits = (bits << 8) + static_cast<std::uint8_t>(character);
        bit_count += 8;
        while (bit_count >= 0) {
            output.push_back(lookup[(bits >> bit_count) & 0x3F]);
            bit_count -= 6;
        }
    }

    if (bit_count > -6) {
        output.push_back(lookup[((bits << 8) >> (bit_count + 8)) & 0x3F]);
    }

    while (output.size() % 4 != 0) {
        output.push_back('=');
    }

    return output;
}

}  // namespace malama::network

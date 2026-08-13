// /////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/token/token_estimator.hpp
// Purpose:     Heuristic and BPE-based token payload estimator for LLM context
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "core/models.hpp"
#include "engine/storage/attachment_manager.hpp"

namespace malama::engine::token {

class TokenEstimator {
   public:
    constexpr TokenEstimator() noexcept = default;

    [[nodiscard]] auto estimate_text_tokens(std::string_view text) const noexcept -> std::size_t;

    [[nodiscard]] auto estimate_payload_tokens(
        std::string_view current_prompt,
        const std::vector<storage::AttachmentInfo> &pending_attachments,
        const std::vector<core::Message> &history) const noexcept -> std::size_t;
};

}  // namespace malama::engine::token

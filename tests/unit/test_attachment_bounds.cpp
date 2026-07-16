// /////////////////////////////////////////////////////////////////////////////
// Name:        tests/unit/test_attachment_bounds.cpp
// Purpose:     Validates attachment manager public queues and bounds error states
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-16
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>

#include "engine/storage/attachment_manager.hpp"

inline constexpr std::size_t idx_lower_bound_limit = 0UZ;
inline constexpr std::size_t idx_upper_bound_limit = 999UZ;

SCENARIO("Attachment manager handles queues and invalid assets safely", "[bounds]") {
    GIVEN("An active AttachmentManager component with an empty queue tracking array") {
        malama::engine::storage::AttachmentManager attachment_manager_instance;

        THEN("The initial tracking array must contain zero elements") {
            const auto &pending_list = attachment_manager_instance.GetPendingAttachments();
            REQUIRE(pending_list.empty() == true);
        }

        WHEN("An out-of-bounds index removal is requested on the empty queue") {
            attachment_manager_instance.RemoveByIndex(idx_lower_bound_limit);
            attachment_manager_instance.RemoveByIndex(idx_upper_bound_limit);

            THEN("The system must degrade gracefully and retain an empty state") {
                const auto &pending_list = attachment_manager_instance.GetPendingAttachments();
                REQUIRE(pending_list.empty() == true);
            }
        }

        WHEN("A non-existent file path is passed to the parser pipeline") {
            const std::string missing_file_path{"missing_unsupported_document.xyz"};
            const auto ingestion_result =
                attachment_manager_instance.AnalyzeAndAdd(missing_file_path);

            THEN("The pipeline must fail and return a structured ingestion error code") {
                REQUIRE(ingestion_result.has_value() == false);

                const malama::engine::storage::IngestionError error_code = ingestion_result.error();

                // Confirm that a valid error code mapping branch was triggered
                const bool is_valid_error =
                    (error_code == malama::engine::storage::IngestionError::FILE_NOT_FOUND) ||
                    (error_code == malama::engine::storage::IngestionError::PARSING_FAILED);

                REQUIRE(is_valid_error == true);
            }
        }

        WHEN("The pending attachments list is cleared explicitly") {
            attachment_manager_instance.ClearQueue();

            THEN("The tracking vector must remain completely empty") {
                const auto &pending_list = attachment_manager_instance.GetPendingAttachments();
                REQUIRE(pending_list.empty() == true);
            }
        }
    }
}

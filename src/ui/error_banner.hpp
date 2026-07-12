// /////////////////////////////////////////////////////////////////////////////
// Name:        include/ui/error_banner.hpp
// Purpose:     Color-coded system notification bar for context and input errors
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3-or-later

#include <cstdint>
#include <string_view>
#include <wx/panel.h>
#include <wx/stattext.h>

namespace malama::ui {

/// @brief Selection states for the visual notification context mapping.
enum class BannerState : std::uint8_t { ERROR, WARNING, INFORMATION };

/// @brief Inline custom window overlay providing real-time capability gating notices.
/// Thread-safety: Must be manipulated exclusively on the main wxWidgets UI thread.
class ErrorBanner final : public wxPanel {
   public:
    explicit ErrorBanner(wxWindow *parent_ptr);
    ~ErrorBanner() override = default;

    ErrorBanner(const ErrorBanner &) = delete;
    ErrorBanner &operator=(const ErrorBanner &) = delete;
    ErrorBanner(ErrorBanner &&) noexcept = delete;
    ErrorBanner &operator=(ErrorBanner &&) noexcept = delete;

    /// @brief Displays an error, warning, or informative banner inline.
    /// @param message Text copy string to show to the end user.
    /// @param state Target alert level configuration dictating background color.
    void ShowAlert(std::string_view message, BannerState state) noexcept;

    /// @brief Dismisses and hides the alert banner bar from layout calculations.
    void HideAlert() noexcept;

   private:
    void OnDismissClicked(wxCommandEvent &event) noexcept;

    wxStaticText *m_message_label_ptr{nullptr};
};

}  // namespace malama::ui

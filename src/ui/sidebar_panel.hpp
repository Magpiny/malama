// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/sidebar_panel.hpp
// Purpose:     Sidebar navigation container for managing historical sessions
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-08
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <wx/panel.h>
#include <wx/listbox.h>

namespace malama::ui {

class SidebarPanel final : public wxPanel {
public:
    explicit SidebarPanel(wxWindow *parent_ptr);
    ~SidebarPanel() override = default;

    SidebarPanel(const SidebarPanel &) = delete;
    SidebarPanel &operator=(const SidebarPanel &) = delete;
    SidebarPanel(SidebarPanel &&) noexcept = delete;
    SidebarPanel &operator=(SidebarPanel &&) noexcept = delete;

private:
    void SetupLayout() noexcept;

    wxListBox *m_history_list_ptr{nullptr};
};

} // namespace malama::ui

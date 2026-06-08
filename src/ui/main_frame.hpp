// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/main_frame.hpp
// Purpose:     Top-level workspace frame containing native menu actions
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-08
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <wx/wx.h>

namespace malama::ui {

// Declare the custom cross-thread token notification event cleanly
wxDECLARE_EVENT(EVT_MALAMA_TOKEN, wxThreadEvent);

class MainFrame final : public wxFrame {
public:
    explicit MainFrame(const wxString &title, const wxPoint &pos, const wxSize &size);
    ~MainFrame() override = default;

    MainFrame(const MainFrame &) = delete;
    MainFrame &operator=(const MainFrame &) = delete;
    MainFrame(MainFrame &&) noexcept = delete;
    MainFrame &operator=(MainFrame &&) noexcept = delete;

private:
    void SetupMenuBar() noexcept;
    void BindActionEvents() noexcept;

    // Menu Item Interaction Actions
    void OnExitAction([[maybe_unused]] wxCommandEvent &event);
    void OnAboutAction([[maybe_unused]] wxCommandEvent &event);
    void OnLicenceAction([[maybe_unused]] wxCommandEvent &event);

    // Thread Notification Consumer - Made static to resolve static analysis metrics
    static void OnTokenReceived(wxThreadEvent &event);

    enum class MenuId : std::uint16_t {
        ExitId = wxID_EXIT,
        AboutId = wxID_ABOUT,
        LicenceId = wxID_HIGHEST + 1
    };
};

} // namespace malama::ui

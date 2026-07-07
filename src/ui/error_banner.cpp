// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/error_banner.cpp
// Purpose:     Implements the color-coded overlay banner layout with dismiss hooks
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#include "ui/error_banner.hpp"

#include <new>
#include <wx/button.h>
#include <wx/sizer.h>

namespace malama::ui {

ErrorBanner::ErrorBanner(wxWindow *parent_ptr)
    : wxPanel(parent_ptr, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE) {
    auto *main_sizer = new (std::nothrow) wxBoxSizer(wxHORIZONTAL);
    m_message_label_ptr = new (std::nothrow) wxStaticText(this, wxID_ANY, "");

    auto *dismiss_btn = new (std::nothrow) wxButton(this, wxID_ANY, "X", wxDefaultPosition,
                                                    wxSize(24, 24), wxBU_EXACTFIT | wxBORDER_NONE);

    if (main_sizer == nullptr || m_message_label_ptr == nullptr || dismiss_btn == nullptr)
        [[unlikely]] {
        delete main_sizer;
        delete m_message_label_ptr;
        delete dismiss_btn;
        return;
    }

    // Configure clean visual tracking margins
    dismiss_btn->SetBackgroundColour(wxColour(0, 0, 0, 0));
    dismiss_btn->SetForegroundColour(wxColour("#FFFFFF"));

    main_sizer->Add(m_message_label_ptr, 1, wxALIGN_CENTER_VERTICAL | wxALL, 6);
    main_sizer->Add(dismiss_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    SetSizer(main_sizer);
    dismiss_btn->Bind(wxEVT_BUTTON, &ErrorBanner::OnDismissClicked, this);

    Hide();
}

void ErrorBanner::ShowAlert(std::string_view message, BannerState state) noexcept {
    if (m_message_label_ptr == nullptr) [[unlikely]] {
        return;
    }

    m_message_label_ptr->SetLabel(wxString::FromUTF8(message.data(), message.size()));

    // Strict non-default enum switch processing mapping your explicit hex design specs
    switch (state) {
        case BannerState::ERROR: {
            SetBackgroundColour(wxColour("#8B0000"));  // Deep Crimson Error Red
            m_message_label_ptr->SetForegroundColour(wxColour("#FFFFFF"));
            break;
        }
        case BannerState::WARNING: {
            SetBackgroundColour(wxColour("#B8860B"));  // Dark Amber/Gold Yellow
            m_message_label_ptr->SetForegroundColour(wxColour("#FFFFFF"));
            break;
        }
        case BannerState::INFORMATION: {
            SetBackgroundColour(wxColour("#2F4F4F"));  // Slate Blue-Green Info Channel
            m_message_label_ptr->SetForegroundColour(wxColour("#F5f5F7"));
            break;
        }
    }

    Show();
    GetParent()->Layout();
}

void ErrorBanner::HideAlert() noexcept {
    Hide();
    GetParent()->Layout();
}

void ErrorBanner::OnDismissClicked(wxCommandEvent &WXUNUSED(event)) noexcept {
    HideAlert();
}

}  // namespace malama::ui

// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/main_frame.cpp
// Purpose:     Implements top-level window controls and menu modal dialog loops
// Author:      Wanjare S. <samuelwanjare@proton.me>
// Created:     2026-06-15
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#include "ui/main_frame.hpp"
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <new>
#include <spdlog/spdlog.h>
#include <string>
#include <wx/aboutdlg.h>
#include <wx/bitmap.h>
#include <wx/event.h>
#include <wx/filename.h>
#include <wx/hyperlink.h>
#include <wx/icon.h>
#include <wx/image.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/string.h>

#include "common/constants.hpp"
#include "config/config_manager.hpp"
#include "engine/export/export_engine.hpp"
#include "ui/chat_panel.hpp"
#include "ui/session_params_dlg.hpp"
#include "ui/settings_dialog.hpp"
#include "ui/sidebar_panel.hpp"

namespace {

[[nodiscard]] malama::common::SessionParameters to_common_params(
    const malama::core::ModelParameters &params) noexcept {
    return malama::common::SessionParameters{.m_temperature = params.m_temperature,
                                             .m_top_p = params.m_top_p,
                                             .m_top_k = static_cast<int>(params.m_top_k),
                                             .m_repeat_penalty = params.m_repeat_penalty,
                                             .m_num_ctx = params.m_num_ctx,
                                             .m_system_prompt = params.m_system_prompt};
}

[[nodiscard]] malama::core::ModelParameters to_core_params(
    const malama::common::SessionParameters &params) noexcept {
    return malama::core::ModelParameters{.m_temperature = params.m_temperature,
                                         .m_top_p = params.m_top_p,
                                         .m_top_k = static_cast<std::int32_t>(params.m_top_k),
                                         .m_repeat_penalty = params.m_repeat_penalty,
                                         .m_num_ctx = params.m_num_ctx,
                                         .m_system_prompt = params.m_system_prompt};
}

}  // namespace

namespace malama::ui {
using std::filesystem::path;

wxDEFINE_EVENT(EVT_MALAMA_TOKEN, wxThreadEvent);
wxDECLARE_EVENT(EVT_USER_PROMPT, wxCommandEvent);

// Scoped menu identifier values matching linkage definitions
inline constexpr int id_menu_preferences = 12001;
inline constexpr int id_menu_session_params = 12002;
inline constexpr int id_menu_exit = 12003;
inline constexpr int id_menu_about = 12004;
inline constexpr int id_menu_licence = 12005;
inline constexpr std::size_t MAX_SESSION_TITLE_LENGTH = 25;

static int s_token_count = 0;
static std::chrono::steady_clock::time_point s_start_time;

static void apply_styles_recursively(wxWindow *window_ptr, const wxColour &bg_color,
                                     const wxColour &surface_color, const wxColour &fg_color,
                                     const wxFont &font) noexcept {
    if (window_ptr == nullptr) {
        return;
    }

    if (window_ptr->IsKindOf(wxCLASSINFO(wxTextCtrl))) {
        window_ptr->SetBackgroundColour(surface_color);
    } else {
        window_ptr->SetBackgroundColour(bg_color);
    }

    window_ptr->SetForegroundColour(fg_color);
    window_ptr->SetFont(font);

    for (auto *child_ptr : window_ptr->GetChildren()) {
        apply_styles_recursively(child_ptr, bg_color, surface_color, fg_color, font);
    }

    window_ptr->Refresh();
}

MainFrame::MainFrame(const wxString &title, const wxPoint &pos, const wxSize &size,
                     std::function<void(const std::string &)> on_prompt_submit)
    : wxFrame(nullptr, wxID_ANY, title, pos, size),
      m_on_prompt_submit_callback(std::move(on_prompt_submit)) {
    const char *home_dir = std::getenv("HOME");
    path app_data_dir = (home_dir != nullptr)
                            ? std::filesystem::path(home_dir) / ".local" / "share" / "malama"
                            : std::filesystem::path("/tmp/malama");

    std::error_code session_dir_error;
    std::filesystem::create_directories(app_data_dir / "sessions", session_dir_error);
    if (session_dir_error) {
        spdlog::error("Failed to create sessions dir: {}", session_dir_error.message());
    }

    m_history_manager_ptr =
        std::make_unique<engine::storage::HistoryManager>(app_data_dir / "sessions");

    setup_menu_bar();
    setup_workspace_layout();
    bind_action_events();

    apply_appearance_settings();
    LoadMostRecentSessionOnStartup();
}

void MainFrame::load_application_icon() noexcept {
    namespace fs = std::filesystem;

    const std::string exe_full_path = wxStandardPaths::Get().GetExecutablePath().ToStdString();
    const fs::path exe_dir = fs::path(exe_full_path).parent_path();

    const std::vector<fs::path> candidate_paths = {
        exe_dir / "assets" / "malama.png", exe_dir / "assets" / "malama.png",
        exe_dir / "malama.png", fs::path("./assets/malama.png")};

    fs::path target_image_path;
    for (const auto &candidate : candidate_paths) {
        if (fs::exists(candidate)) {
            target_image_path = candidate;
            break;
        }
    }

    if (target_image_path.empty()) {
        spdlog::warn("Icon file not found in build assets. Checked path: {}",
                     (exe_dir / "assets").string());
        return;
    }

    wxImage logo_image;
    if (logo_image.LoadFile(wxString::FromUTF8(target_image_path.string()), wxBITMAP_TYPE_PNG) &&
        logo_image.IsOk()) {
        wxBitmap logo_bitmap(logo_image);
        wxIcon app_icon;
        app_icon.CopyFromBitmap(logo_bitmap);

        SetIcon(app_icon);
        spdlog::info("Successfully loaded frame icon from: {}", target_image_path.string());
    } else {
        spdlog::error("Failed to decode PNG image buffer at: {}", target_image_path.string());
    }
}

auto MainFrame::AppendUserMessage(std::string_view message) noexcept -> void {
    if (m_chat_panel_ptr != nullptr) {
        m_chat_panel_ptr->append_user_message(message);
        m_chat_panel_ptr->start_spinner();
    }
    s_token_count = 0;
    s_start_time = std::chrono::steady_clock::now();
}

auto MainFrame::AppendToken(std::string_view token_segment) noexcept -> void {
    if (m_chat_panel_ptr != nullptr) {
        m_chat_panel_ptr->append_token(token_segment);
    }

    s_token_count++;
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = now - s_start_time;
    double tps = 0.0;
    if (duration.count() > 0.0) {
        tps = static_cast<double>(s_token_count) / duration.count();
    }

    double denominator = 200.0;
    double exponent = -static_cast<double>(s_token_count) / denominator;
    double progress = 100.0 * (1.0 - std::exp(exponent));
    int percentage = std::min(99, static_cast<int>(progress));

    if (m_sidebar_panel_ptr != nullptr) {
        m_sidebar_panel_ptr->update_metrics(tps, percentage);
    }
}

void MainFrame::FinalizeAssistantResponse() noexcept {
    if (m_chat_panel_ptr != nullptr) {
        m_chat_panel_ptr->stop_spinner();
    }
    if (m_sidebar_panel_ptr != nullptr) {
        m_sidebar_panel_ptr->update_metrics(0.0, 100);
    }

    if (m_chat_panel_ptr == nullptr || m_history_manager_ptr == nullptr) {
        return;
    }
    if (m_current_session_id.empty()) {
        return;
    }

    std::string response_text = m_chat_panel_ptr->get_active_response_stream();
    if (response_text.empty()) {
        return;
    }

    core::Message assistant_message;
    assistant_message.m_id = engine::storage::HistoryManager::GenerateUuidString();
    assistant_message.m_role = core::MessageRole::Assistant;
    assistant_message.m_content = response_text;
    assistant_message.m_timestamp = engine::storage::HistoryManager::GetCurrentEpoch();
    assistant_message.m_is_starred = false;

    m_history_manager_ptr->AppendMessage(m_current_session_id, assistant_message);

    if (m_sidebar_panel_ptr != nullptr) {
        m_sidebar_panel_ptr->populate_sidebar();
        m_sidebar_panel_ptr->select_session_by_id(m_current_session_id);
    }
}

void MainFrame::LoadMostRecentSessionOnStartup() noexcept {
    if (m_history_manager_ptr == nullptr) {
        return;
    }

    auto sessions_list = m_history_manager_ptr->LoadAllMetadata();
    if (sessions_list.empty()) {
        return;
    }

    std::ranges::sort(sessions_list,
                      [](const core::SessionMetadata &lhs, const core::SessionMetadata &rhs) {
                          if (lhs.m_is_pinned != rhs.m_is_pinned) {
                              return lhs.m_is_pinned > rhs.m_is_pinned;
                          }
                          return lhs.m_updated_at > rhs.m_updated_at;
                      });

    m_current_session_id = sessions_list.front().m_session_id;
    auto active_session = m_history_manager_ptr->LoadSession(m_current_session_id);

    if (active_session.has_value() && m_chat_panel_ptr != nullptr) {
        m_current_session_params = to_common_params(active_session->m_metadata.m_parameters);
        m_chat_panel_ptr->load_history(active_session.value());

        if (m_sidebar_panel_ptr != nullptr) {
            m_sidebar_panel_ptr->select_session_by_id(m_current_session_id);
        }
    }
}

void MainFrame::setup_menu_bar() noexcept {
    auto *menu_bar_ptr = new (std::nothrow) wxMenuBar();
    if (menu_bar_ptr == nullptr) {
        return;
    }

    // 1. File Menu
    auto *file_menu_ptr = new (std::nothrow) wxMenu();
    if (file_menu_ptr == nullptr) {
        delete menu_bar_ptr;
        return;
    }

    file_menu_ptr->Append(id_menu_preferences, "Global Settings...\tCtrl+,",
                          "Configure application settings");
    file_menu_ptr->AppendSeparator();
    file_menu_ptr->Append(id_menu_exit, "Exit\tAlt-X", "Terminate application framework");
    menu_bar_ptr->Append(file_menu_ptr, "&File");

    // 2. Session Menu (Parameters & System Prompt overrides)
    auto *session_menu_ptr = new (std::nothrow) wxMenu();
    if (session_menu_ptr != nullptr) {
        session_menu_ptr->Append(id_menu_session_params, "Session Parameters...\tCtrl+Shift+P",
                                 "Tune temperature, context size (num_ctx), and system prompt");
        menu_bar_ptr->Append(session_menu_ptr, "&Session");
    }

    // 3. Help Menu
    auto *help_menu_ptr = new (std::nothrow) wxMenu();
    if (help_menu_ptr == nullptr) {
        delete menu_bar_ptr;
        return;
    }
    help_menu_ptr->Append(id_menu_licence, "Licence", "Display licensing constraints");
    help_menu_ptr->Append(id_menu_about, "About...", "Display platform implementation details");
    menu_bar_ptr->Append(help_menu_ptr, "&Help");

    SetMenuBar(menu_bar_ptr);
}

void MainFrame::setup_workspace_layout() noexcept {
    auto *splitter = new (std::nothrow) wxSplitterWindow(this, wxID_ANY, wxDefaultPosition,
                                                         wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D);

    m_splitter_window_ptr = splitter;
    if (m_splitter_window_ptr == nullptr) {
        return;
    }

    auto *sidebar =
        new (std::nothrow) SidebarPanel(m_splitter_window_ptr, m_history_manager_ptr.get());
    m_sidebar_panel_ptr = sidebar;
    if (m_sidebar_panel_ptr == nullptr) {
        delete m_splitter_window_ptr;
        return;
    }

    auto *chat = new (std::nothrow) ChatPanel(m_splitter_window_ptr);
    m_chat_panel_ptr = chat;
    if (m_chat_panel_ptr == nullptr) {
        delete m_sidebar_panel_ptr;
        delete m_splitter_window_ptr;
        return;
    }

    m_splitter_window_ptr->SetMinimumPaneSize(constants::minimum_pane_size_pixels);
    m_splitter_window_ptr->SplitVertically(m_sidebar_panel_ptr, m_chat_panel_ptr,
                                           constants::default_sash_position);
}

void MainFrame::bind_action_events() noexcept {
    Bind(wxEVT_MENU, &MainFrame::on_preferences_action, this, id_menu_preferences);
    Bind(wxEVT_MENU, &MainFrame::on_session_params_action, this, id_menu_session_params);
    Bind(wxEVT_MENU, &MainFrame::on_exit_action, this, id_menu_exit);
    Bind(wxEVT_MENU, &MainFrame::on_about_action, this, id_menu_about);
    Bind(wxEVT_MENU, &MainFrame::on_licence_action, this, id_menu_licence);
    Bind(EVT_USER_PROMPT, &MainFrame::on_user_prompt_submitted, this);
    Bind(EVT_LOAD_SESSION, &MainFrame::on_load_session, this);
    Bind(EVT_NEW_CHAT_REQUESTED, &MainFrame::on_new_chat_action, this);

    Bind(EVT_CANCEL_GENERATION, [this](wxCommandEvent &WXUNUSED(event)) {
        spdlog::warn("Asynchronous LLM computation stream terminated by the user.");
        if (m_sidebar_panel_ptr != nullptr) {
            m_sidebar_panel_ptr->clear_metrics();
        }
    });
}

void MainFrame::on_load_session(wxCommandEvent &event) noexcept {
    m_current_session_id = event.GetString().ToStdString();

    if (m_history_manager_ptr && (m_chat_panel_ptr != nullptr)) {
        auto session_opt = m_history_manager_ptr->LoadSession(m_current_session_id);
        if (session_opt.has_value()) {
            m_current_session_params = to_common_params(session_opt->m_metadata.m_parameters);
            m_chat_panel_ptr->load_history(session_opt.value());
        }
    }
}

void MainFrame::apply_appearance_settings() noexcept {
    const auto app_config = config::ConfigManager::get_instance().get_config();
    const auto &visual_props = app_config.m_appearance;

    wxColour sidebar_bg_color(wxString::FromUTF8(visual_props.m_sidebar_bg));
    wxColour sidebar_text_color(wxString::FromUTF8(visual_props.m_sidebar_text));
    wxColour main_bg_color(wxString::FromUTF8(visual_props.m_bg_color));
    wxColour surface_color(wxString::FromUTF8(visual_props.m_surface_color));
    wxColour main_text_color(wxString::FromUTF8(visual_props.m_text_primary));

    wxFont system_ui_font(wxFontInfo(visual_props.m_font_size)
                              .FaceName(wxString::FromUTF8(visual_props.m_font_family)));

    if (m_sidebar_panel_ptr != nullptr) {
        apply_styles_recursively(m_sidebar_panel_ptr, sidebar_bg_color, surface_color,
                                 sidebar_text_color, system_ui_font);
    }

    if (m_chat_panel_ptr != nullptr) {
        apply_styles_recursively(m_chat_panel_ptr, main_bg_color, surface_color, main_text_color,
                                 system_ui_font);
    }

    this->Refresh();
}

void MainFrame::on_preferences_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    SettingsDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        apply_appearance_settings();
    }
}

void MainFrame::on_session_params_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    if (m_chat_panel_ptr != nullptr) {
        const auto &active_session = m_chat_panel_ptr->get_active_session();
        if (!active_session.m_metadata.m_session_id.empty()) {
            m_current_session_params = to_common_params(active_session.m_metadata.m_parameters);
        }
    }

    SessionParamsDialog dialog(this, m_current_session_params);
    if (dialog.ShowModal() == wxID_OK) {
        m_current_session_params = dialog.GetParameters();

        // 1. Update live ChatPanel in-memory session parameters
        if (m_chat_panel_ptr != nullptr) {
            m_chat_panel_ptr->set_session_parameters(m_current_session_params);
        }

        // 2. Persist updated parameters to SQLite database via core::ModelParameters conversion
        if (m_history_manager_ptr != nullptr && !m_current_session_id.empty()) {
            m_history_manager_ptr->UpdateSessionParameters(
                m_current_session_id, to_core_params(m_current_session_params));
        }

        spdlog::info("Updated thread session parameters: temperature={:.2f}, num_ctx={}",
                     m_current_session_params.m_temperature, m_current_session_params.m_num_ctx);
    }
}

void MainFrame::on_exit_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    Close(true);
}

void MainFrame::on_about_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    wxAboutDialogInfo info;
    info.SetName(_("Malama"));
    info.SetVersion(_("0.3.0"));
    info.SetDescription(
        wxT("Native Linux chat client for local LLMs — no cloud, no browser, no compromise."));
    info.SetCopyright(wxT("Copyright (C) 2026"));
    info.SetWebSite(wxT("https://magpiny.dev"));
    info.AddDeveloper(wxT("Wanjare Samuel"));

    wxIcon logo_icon;
    if (wxFileExists(wxT("assets/malama.png"))) {
        logo_icon.LoadFile(wxT("assets/malama.png"), wxBITMAP_TYPE_PNG);
    } else if (wxFileExists(wxT("malama.png"))) {
        logo_icon.LoadFile(wxT("malama.png"), wxBITMAP_TYPE_PNG);
    } else if (wxFileExists(wxT("/usr/share/icons/hicolor/256x256/apps/malama.png"))) {
        logo_icon.LoadFile(wxT("/usr/share/icons/hicolor/256x256/apps/malama.png"),
                           wxBITMAP_TYPE_PNG);
    }

    if (logo_icon.IsOk()) {
        info.SetIcon(logo_icon);
    }

    wxAboutBox(info, this);
}

enum class LicenceDialogError : std::uint8_t {
    allocation_failure,
    display_failure,
};

[[nodiscard]] constexpr std::string_view to_string(LicenceDialogError err) noexcept {
    switch (err) {
        case LicenceDialogError::allocation_failure:
            return "Not enough memory to open dialog.";
        case LicenceDialogError::display_failure:
            return "Could not display the license dialog.";
    }
    return "Unknown error.";
}

[[nodiscard]] std::expected<void, LicenceDialogError> show_licence_dialog(
    wxWindow *parent) noexcept {
    static constexpr int dialog_width = 400;
    static constexpr int dialog_height = 320;
    static constexpr int content_padding = 15;
    static constexpr int button_padding = 10;
    static constexpr int wrap_width = dialog_width - (2 * content_padding);

    try {
        wxDialog dlg(parent, wxID_ANY, _("License"), wxDefaultPosition,
                     wxSize(dialog_width, dialog_height), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

        auto sizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

        auto *header =
            new wxStaticText(&dlg, wxID_ANY, _("GNU General Public License v3.0 or later"));
        if (header == nullptr) {
            return std::unexpected(LicenceDialogError::allocation_failure);
        }
        header->SetFont(header->GetFont().Bold());
        header->Wrap(wrap_width);
        sizer->Add(header, 0, wxEXPAND | wxALL, content_padding);

        const wxString body =
            _("You are free to share and modify this software, provided that "
              "all derivative works remain open source under the same license "
              "terms.\n\nNo warranty is provided.");
        auto *body_text = new wxStaticText(&dlg, wxID_ANY, body);
        if (body_text == nullptr) {
            return std::unexpected(LicenceDialogError::allocation_failure);
        }
        body_text->Wrap(wrap_width);
        sizer->Add(body_text, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, content_padding);

        auto *link = new wxHyperlinkCtrl(&dlg, wxID_ANY, _("View full license text (gnu.org)"),
                                         _("https://www.gnu.org/licenses/gpl-3.0.html"));
        if (link == nullptr) {
            return std::unexpected(LicenceDialogError::allocation_failure);
        }
        sizer->Add(link, 0, wxALIGN_CENTER | wxBOTTOM, content_padding);

        auto btn_sizer = std::make_unique<wxStdDialogButtonSizer>();
        auto *close_btn = new wxButton(&dlg, wxID_OK, _("Close"));
        if (close_btn == nullptr) {
            return std::unexpected(LicenceDialogError::allocation_failure);
        }
        btn_sizer->AddButton(close_btn);
        btn_sizer->Realize();
        sizer->Add(btn_sizer.release(), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, button_padding);

        dlg.SetSizer(sizer.release());
        dlg.ShowModal();
        return {};
    } catch (const std::bad_alloc &) {
        return std::unexpected(LicenceDialogError::allocation_failure);
    } catch (...) {
        return std::unexpected(LicenceDialogError::display_failure);
    }
}

void MainFrame::on_licence_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    if (const auto result = show_licence_dialog(this); !result) {
        spdlog::error("{}", to_string(result.error()));
    }
}

void MainFrame::on_user_prompt_submitted(wxCommandEvent &event) noexcept {
    if (m_history_manager_ptr == nullptr) {
        return;
    }

    std::string prompt_text = event.GetString().ToStdString(wxConvUTF8);
    if (prompt_text.empty()) {
        return;
    }

    if (m_current_session_id.empty()) {
        std::string default_title = prompt_text.length() > MAX_SESSION_TITLE_LENGTH
                                        ? prompt_text.substr(0, MAX_SESSION_TITLE_LENGTH) + "..."
                                        : prompt_text;

        auto new_session = m_history_manager_ptr->CreateSession(default_title);
        m_current_session_id = new_session.m_session_id;

        // Persist tuned parameters to newly created session
        m_history_manager_ptr->UpdateSessionParameters(m_current_session_id,
                                                       to_core_params(m_current_session_params));

        if (m_sidebar_panel_ptr != nullptr) {
            m_sidebar_panel_ptr->populate_sidebar();
            m_sidebar_panel_ptr->select_session_by_id(m_current_session_id);
        }
    }

    core::Message user_msg;
    user_msg.m_id = engine::storage::HistoryManager::GenerateUuidString();
    user_msg.m_role = core::MessageRole::User;
    user_msg.m_content = prompt_text;
    user_msg.m_timestamp = engine::storage::HistoryManager::GetCurrentEpoch();
    user_msg.m_is_starred = false;

    m_history_manager_ptr->AppendMessage(m_current_session_id, user_msg);

    if (m_on_prompt_submit_callback) {
        m_on_prompt_submit_callback(prompt_text);
    }
}

void MainFrame::on_new_chat_action(wxCommandEvent &WXUNUSED(event)) noexcept {
    m_current_session_id.clear();
    m_current_session_params = common::SessionParameters{};
    if (m_chat_panel_ptr != nullptr) {
        m_chat_panel_ptr->load_history(core::ChatSession{});
    }
}

auto MainFrame::GetActiveSessionMessages() const noexcept -> std::vector<core::Message> {
    if (m_chat_panel_ptr == nullptr) {
        return {};
    }
    return m_chat_panel_ptr->get_active_session().m_messages;
}

void MainFrame::on_export_session_action(wxCommandEvent &WXUNUSED(event)) {
    if (m_chat_panel_ptr == nullptr) {
        return;
    }

    const auto &active_session = m_chat_panel_ptr->get_active_session();

    wxDirDialog dir_dialog(this, _("Select Export Directory"), wxGetHomeDir(),
                           wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dir_dialog.ShowModal() != wxID_OK) {
        return;
    }

    std::filesystem::path target_dir = dir_dialog.GetPath().ToStdString();

    malama::engine::export_sys::ExportEngine engine;
    auto result = engine.export_session(active_session, target_dir,
                                        malama::engine::export_sys::ExportFormat::Markdown);

    if (result.has_value()) {
        wxLogMessage(_("Successfully exported thread to: %s"), result.value().string().c_str());
    } else {
        wxLogError(_("Export failed with error code: %d"), static_cast<int>(result.error()));
    }
}

}  // namespace malama::ui

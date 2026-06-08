// /////////////////////////////////////////////////////////////////////////////
// Name:        tests/test_network_ui.cpp
// Purpose:     Unit tests for EVT_MALAMA_TOKEN cross-thread event and UI token
//              handling logic added in v0.0.4 thread bridge implementation
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-08
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <wx/app.h>
#include <wx/event.h>
#include <wx/strconv.h>
#include <wx/string.h>

#include "ui/main_frame.hpp"

// ---------------------------------------------------------------------------
// Minimal wxWidgets environment initialiser required for wxString UTF-8 ops
// ---------------------------------------------------------------------------
class WxTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        m_argc = 1;
        m_argv[0] = const_cast<char *>("malama_test"); // NOLINT
        m_argv[1] = nullptr;
        wxApp::SetInstance(new wxApp()); // NOLINT(cppcoreguidelines-owning-memory)
        wxEntryStart(m_argc, m_argv);
    }

    void TearDown() override {
        wxEntryCleanup();
    }

private:
    int m_argc{0};
    char *m_argv[2]{};
};

// ---------------------------------------------------------------------------
// EVT_MALAMA_TOKEN event-type registration tests
// Validates the wxDECLARE_EVENT / wxDEFINE_EVENT pair introduced in the PR.
// ---------------------------------------------------------------------------

TEST(EVTMalamaToken, EventTypeIdIsNotNull) {
    // wxEVT_NULL (-1) is the sentinel for an uninitialised event type.
    // A properly registered event via wxDEFINE_EVENT must never equal wxEVT_NULL.
    EXPECT_NE(malama::ui::EVT_MALAMA_TOKEN.GetEventType(), wxEVT_NULL);
}

TEST(EVTMalamaToken, EventTypeIdIsPositive) {
    // All dynamically allocated wx event IDs are positive integers.
    EXPECT_GT(malama::ui::EVT_MALAMA_TOKEN.GetEventType(), 0);
}

TEST(EVTMalamaToken, EventTypeIsDistinctFromMenuEvent) {
    // EVT_MALAMA_TOKEN must not collide with wxEVT_MENU to prevent misrouting.
    EXPECT_NE(malama::ui::EVT_MALAMA_TOKEN, wxEVT_MENU);
}

TEST(EVTMalamaToken, EventTypeIsDistinctFromBuiltinThreadEvent) {
    // wxEVT_THREAD is the built-in thread notification; the custom event must differ.
    EXPECT_NE(malama::ui::EVT_MALAMA_TOKEN, wxEVT_THREAD);
}

TEST(EVTMalamaToken, EventTypeIsDistinctFromButtonEvent) {
    EXPECT_NE(malama::ui::EVT_MALAMA_TOKEN, wxEVT_BUTTON);
}

TEST(EVTMalamaToken, RepeatedAccessReturnsSameId) {
    // The same global wxEventTypeTag must produce the same numeric ID each call.
    const wxEventType first_id = malama::ui::EVT_MALAMA_TOKEN.GetEventType();
    const wxEventType second_id = malama::ui::EVT_MALAMA_TOKEN.GetEventType();
    EXPECT_EQ(first_id, second_id);
}

// ---------------------------------------------------------------------------
// wxThreadEvent construction and string payload tests
// Validates the wxQueueEvent pipeline introduced in MalamaApp::OnInit.
// ---------------------------------------------------------------------------

TEST(ThreadEventPayload, ConstructingEventWithTokenTypeSucceeds) {
    // Mirrors: new (std::nothrow) wxThreadEvent(ui::EVT_MALAMA_TOKEN)
    const wxThreadEvent evt(malama::ui::EVT_MALAMA_TOKEN);
    EXPECT_EQ(evt.GetEventType(), malama::ui::EVT_MALAMA_TOKEN.GetEventType());
}

TEST(ThreadEventPayload, AsciiStringRoundTrip) {
    // Mirrors: event_ptr->SetString(wxString::FromUTF8(token.data(), token.size()))
    wxThreadEvent evt(malama::ui::EVT_MALAMA_TOKEN);
    const std::string original{"hello malama"};
    evt.SetString(wxString::FromUTF8(original.data(), original.size()));
    const std::string recovered = evt.GetString().ToStdString(wxConvUTF8);
    EXPECT_EQ(recovered, original);
}

TEST(ThreadEventPayload, EmptyTokenRoundTrip) {
    // Validates that a zero-length streaming token is handled without truncation.
    wxThreadEvent evt(malama::ui::EVT_MALAMA_TOKEN);
    const std::string original{};
    evt.SetString(wxString::FromUTF8(original.data(), original.size()));
    const std::string recovered = evt.GetString().ToStdString(wxConvUTF8);
    EXPECT_EQ(recovered, original);
    EXPECT_TRUE(recovered.empty());
}

TEST(ThreadEventPayload, MultibyteUTF8TokenRoundTrip) {
    // Validates that non-ASCII LLM output tokens (accented chars, etc.) survive
    // the wxString::FromUTF8 -> ToStdString(wxConvUTF8) round-trip in
    // OnTokenReceived.
    wxThreadEvent evt(malama::ui::EVT_MALAMA_TOKEN);
    const std::string original{"\xc3\xa9\xc3\xa0\xc3\xbc"}; // é à ü in UTF-8
    evt.SetString(wxString::FromUTF8(original.data(), original.size()));
    const std::string recovered = evt.GetString().ToStdString(wxConvUTF8);
    EXPECT_EQ(recovered, original);
}

TEST(ThreadEventPayload, WhitespaceAndNewlineTokenRoundTrip) {
    // LLM streaming tokens frequently include newlines and spaces.
    wxThreadEvent evt(malama::ui::EVT_MALAMA_TOKEN);
    const std::string original{" \n\t "};
    evt.SetString(wxString::FromUTF8(original.data(), original.size()));
    const std::string recovered = evt.GetString().ToStdString(wxConvUTF8);
    EXPECT_EQ(recovered, original);
}

TEST(ThreadEventPayload, LargeTokenPayloadRoundTrip) {
    // A single streamed token could span many bytes; ensure no truncation occurs.
    wxThreadEvent evt(malama::ui::EVT_MALAMA_TOKEN);
    const std::string original(512, 'x');
    evt.SetString(wxString::FromUTF8(original.data(), original.size()));
    const std::string recovered = evt.GetString().ToStdString(wxConvUTF8);
    EXPECT_EQ(recovered, original);
    EXPECT_EQ(recovered.size(), 512u);
}

TEST(ThreadEventPayload, NullptrGuardAllocationSucceedsUnderNormalConditions) {
    // Regression: PR diff adds null guard on event_ptr allocation:
    //   auto *event_ptr = new (std::nothrow) wxThreadEvent(ui::EVT_MALAMA_TOKEN);
    //   if (event_ptr) { ... }
    // Under normal memory conditions the allocation must succeed and return non-null,
    // confirming that the null-guard path is an exceptional branch only.
    auto *event_ptr = new (std::nothrow) wxThreadEvent(malama::ui::EVT_MALAMA_TOKEN);
    ASSERT_NE(event_ptr, nullptr);
    delete event_ptr; // NOLINT(cppcoreguidelines-owning-memory)
}

// ---------------------------------------------------------------------------
// OnTokenReceived static method contract tests
// The body is: event.GetString().ToStdString(wxConvUTF8) + spdlog::trace.
// We validate the UTF-8 conversion contract used in the production handler.
// ---------------------------------------------------------------------------

TEST(OnTokenReceivedBehaviour, AsciiPayloadConversionIsCorrect) {
    wxThreadEvent evt(malama::ui::EVT_MALAMA_TOKEN);
    evt.SetString(wxString::FromUTF8("streaming_token"));
    const auto result = evt.GetString().ToStdString(wxConvUTF8);
    EXPECT_EQ(result, "streaming_token");
}

TEST(OnTokenReceivedBehaviour, EmptyPayloadConversionIsCorrect) {
    wxThreadEvent evt(malama::ui::EVT_MALAMA_TOKEN);
    evt.SetString(wxString{});
    const auto result = evt.GetString().ToStdString(wxConvUTF8);
    EXPECT_TRUE(result.empty());
}

TEST(OnTokenReceivedBehaviour, ASCIIBoundaryByteConversion) {
    // Boundary regression: DEL character (0x7F) is the last single-byte ASCII value.
    wxThreadEvent evt(malama::ui::EVT_MALAMA_TOKEN);
    const std::string boundary{"\x7f"};
    evt.SetString(wxString::FromUTF8(boundary.data(), boundary.size()));
    const std::string result = evt.GetString().ToStdString(wxConvUTF8);
    EXPECT_EQ(result, boundary);
    EXPECT_EQ(static_cast<unsigned char>(result[0]), 0x7Fu);
}

TEST(OnTokenReceivedBehaviour, ThreeByteUTF8Sequence) {
    // CJK character U+4E2D (中) encoded as 3-byte UTF-8 sequence \xE4\xB8\xAD.
    wxThreadEvent evt(malama::ui::EVT_MALAMA_TOKEN);
    const std::string original{"\xe4\xb8\xad"};
    evt.SetString(wxString::FromUTF8(original.data(), original.size()));
    const std::string result = evt.GetString().ToStdString(wxConvUTF8);
    EXPECT_EQ(result, original);
    EXPECT_EQ(result.size(), 3u);
}

// ---------------------------------------------------------------------------
// Test main — registers the wxWidgets environment before running all suites
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new WxTestEnvironment()); // NOLINT
    return RUN_ALL_TESTS();
}

// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest
#pragma once

#include <memory>
#include <optional>

#include <iso15118/config.hpp>

#include <iso15118/d20/config.hpp>
#include <iso15118/d20/context.hpp>
#include <iso15118/d20/control_event_queue.hpp>
#include <iso15118/d20/states.hpp>
#include <iso15118/fsm/fsm.hpp>

#include <iso15118/io/connection_abstract.hpp>
#include <iso15118/io/poll_manager.hpp>
#include <iso15118/io/sdp_packet.hpp>
#include <iso15118/io/time.hpp>

#include <iso15118/session/feedback.hpp>
#include <iso15118/session/logger.hpp>

#include <iso15118/d20/timeout.hpp>

namespace iso15118 {

// Lightweight session runtime state flags used by Session::poll().
struct SessionState {
    bool connected{false};
    bool new_data{false};
    bool fsm_needs_call{false};
};

// ISO 15118-20 EVSE session driver.
// Owns the transport connection, protocol context, and state machine.
// Typical usage: construct with a connection + SessionConfig + callbacks,
// then call poll() periodically from an event loop.
class Session {
public:
    // connection: transport (plain TCP or TLS) used for V2GTP payloads
    // session_config: EVSE capabilities and limits (derived from EvseSetupConfig)
    // callbacks: feedback hooks for external integration
    // pause_ctx: optional persisted pause context for session resume
    Session(std::unique_ptr<io::IConnection>, d20::SessionConfig, const session::feedback::Callbacks&,
            std::optional<d20::PauseContext>&);
    ~Session();

    // Advance IO + state machine work; returns the next desired wakeup time.
    TimePoint const& poll();
    // Inject control events (e.g., updated limits or mode parameters).
    void push_control_event(const d20::ControlEvent&);

    bool is_finished() const {
        return (ctx.session_stopped or ctx.session_paused);
    }

    // Force-close the connection and terminate the session.
    void close();

private:
    std::unique_ptr<io::IConnection> connection;
    session::SessionLogger log;

    SessionState state;
    // input buffer
    io::SdpPacket packet;

    // output buffer
    uint8_t response_buffer[1028];

    d20::MessageExchange message_exchange{{response_buffer + io::SdpPacket::V2GTP_HEADER_SIZE,
                                           sizeof(response_buffer) - io::SdpPacket::V2GTP_HEADER_SIZE}};

    // Non-blocking delayed close per [V2G20-1643].
    // When session_stopped or session_paused is set, we record the wall-clock
    // time of the trigger and defer the actual close + dlink signal by 5 seconds
    // so the polling loop is not blocked.
    std::optional<TimePoint> delayed_close_at{std::nullopt};

    // control event buffer
    d20::ControlEventQueue control_event_queue;
    d20::Context ctx;

    fsm::v2::FSM<d20::StateBase> fsm;

    TimePoint next_session_event;

    d20::Timeouts timeouts;

    // Receives low-level connection events (accept/open/data/close).
    void handle_connection_event(io::ConnectionEvent event);
};

} // namespace iso15118
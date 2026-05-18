// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest
#include <iso15118/session/iso.hpp>

#include <cassert>
#include <cstring>

#include <endian.h>
#include <iso15118/d20/state/supported_app_protocol.hpp>

#include <iso15118/detail/helper.hpp>

namespace iso15118 {

static constexpr auto SESSION_IDLE_TIMEOUT_MS = 5000;

// Log a raw V2GTP payload for debugging (hex-escaped).
static void log_sdp_packet(const iso15118::io::SdpPacket& sdp) {
    static constexpr auto ESCAPED_BYTE_CHAR_COUNT = 4;
    auto payload_string_buffer = std::make_unique<char[]>(sdp.get_payload_length() * ESCAPED_BYTE_CHAR_COUNT + 1);
    for (std::size_t i = 0; i < sdp.get_payload_length(); ++i) {
        snprintf(payload_string_buffer.get() + i * ESCAPED_BYTE_CHAR_COUNT, ESCAPED_BYTE_CHAR_COUNT + 1, "\\x%02x",
                 static_cast<unsigned int>(sdp.get_payload_buffer()[i]));
    }

    iso15118::logf_info("[SDP Packet in]: Header: %04hx, Payload: %s", sdp.get_payload_type(),
                        payload_string_buffer.get());
}

// Emit a decoded EXI payload into the session log callback.
static void log_packet_from_car(const iso15118::io::SdpPacket& packet, session::SessionLogger& logger) {
    logger.exi(static_cast<uint16_t>(packet.get_payload_type()), packet.get_payload_buffer(),
               packet.get_payload_length(), session::logging::ExiMessageDirection::FROM_EV);
}

// Decode the V2GTP payload into a message variant (SAP / ISO20 main / AC / DC).
static std::unique_ptr<message_20::Variant> make_variant_from_packet(const iso15118::io::SdpPacket& packet) {
    return std::make_unique<message_20::Variant>(
        packet.get_payload_type(), io::StreamInputView{packet.get_payload_buffer(), packet.get_payload_length()});
}

// Log an SDP parsing error and reset the packet so the caller discards it.
static void log_invalid_packet_error(io::SdpPacket& sdp_packet) {
    using PacketState = io::SdpPacket::State;
    switch (sdp_packet.get_state()) {
    case PacketState::INVALID_HEADER:
        logf_warning("Invalid SDP packet header received from vehicle");
        break;
    case PacketState::PAYLOAD_TO_LONG:
        logf_warning("SDP payload too large for buffer");
        break;
    default:
        break;
    }
    sdp_packet = {};
}

// NOTE (aw): this function return true, if it would block to read a complete packet
//            if it returns false, the packet is complete
bool read_single_sdp_packet(io::IConnection& connection, io::SdpPacket& sdp_packet) {
    // NOTE (aw): not happy with this function
    //            main problem is, that it combines too much logic of the sdp packet and io related stuff
    using PacketState = io::SdpPacket::State;

    assert(sdp_packet.get_state() == PacketState::EMPTY || sdp_packet.get_state() == PacketState::HEADER_READ);

    const auto first_try =
        connection.read(sdp_packet.get_current_buffer_pos(), sdp_packet.get_remaining_bytes_to_read());

    sdp_packet.update_read_bytes(first_try.bytes_read);

    if (first_try.would_block) {
        // need more data for at least the header
        return true;
    }

    if (sdp_packet.get_state() == PacketState::COMPLETE) {
        // done
        return false;
    }

    // packet not finished
    if (sdp_packet.get_state() != PacketState::HEADER_READ) {
        log_invalid_packet_error(sdp_packet);
        return false;
    }

    // header read successfully, try to read the rest
    const auto second_try =
        connection.read(sdp_packet.get_current_buffer_pos(), sdp_packet.get_remaining_bytes_to_read());

    sdp_packet.update_read_bytes(second_try.bytes_read);

    if (second_try.would_block) {
        // need more data for the rest of the packet!
        return true;
    }

    // assert finished packet
    if (sdp_packet.get_state() != PacketState::COMPLETE) {
        log_invalid_packet_error(sdp_packet);
        return false;
    }

    return false;
}

// Write a V2GTP response header into the response buffer and return total bytes to send.
static size_t setup_response_header(uint8_t* buffer, iso15118::io::v2gtp::PayloadType payload_type, size_t size) {
    buffer[0] = iso15118::io::SDP_PROTOCOL_VERSION;
    buffer[1] = iso15118::io::SDP_INVERSE_PROTOCOL_VERSION;

    const uint16_t response_payload_type =
        htobe16(static_cast<std::underlying_type_t<iso15118::io::v2gtp::PayloadType>>(payload_type));

    std::memcpy(buffer + 2, &response_payload_type, sizeof(response_payload_type));

    const uint32_t tmp32 = htobe32(size);

    std::memcpy(buffer + 4, &tmp32, sizeof(tmp32));

    return size + iso15118::io::SdpPacket::V2GTP_HEADER_SIZE;
}

Session::Session(std::unique_ptr<io::IConnection> connection_, d20::SessionConfig session_config,
                 const session::feedback::Callbacks& callbacks, std::optional<d20::PauseContext>& pause_ctx) :
    connection(std::move(connection_)),
    log(this),
    ctx(callbacks, log, std::move(session_config), pause_ctx, message_exchange, timeouts),
    fsm(ctx.create_state<d20::state::SupportedAppProtocol>()) {

    next_session_event = offset_time_point_by_ms(get_current_time_point(), SESSION_IDLE_TIMEOUT_MS);
    connection->set_event_callback([this](io::ConnectionEvent event) { this->handle_connection_event(event); });
}

Session::~Session() = default;

void Session::push_control_event(const d20::ControlEvent& event) {
    control_event_queue.push(event);
}

TimePoint const& Session::poll() {
    const auto now = get_current_time_point();

    if (not state.connected) {
        // No active transport connection yet; just schedule a future wakeup.
        next_session_event = offset_time_point_by_ms(now, SESSION_IDLE_TIMEOUT_MS);
        return next_session_event;
    }

    // check for new data to read
    if (state.new_data) {
        const bool would_block = read_single_sdp_packet(*connection, packet);

        if (would_block) {
            state.new_data = false;
        }
    }

    // send all of our queued control events
    while (auto event = control_event_queue.pop()) {
        ctx.set_control_event(std::move(*event));

        if (const auto control_data = ctx.get_control_event<d20::DcTransferLimits>()) {
            ctx.session_config.dc_limits = *control_data;
        } else if (const auto control_data = ctx.get_control_event<d20::EnergyServices>()) {
            ctx.session_config.supported_energy_transfer_services = *control_data;
        } else if (const auto control_data = ctx.get_control_event<d20::SupportedVASs>()) {
            ctx.session_config.supported_vas_services = *control_data;
        } else if (const auto control_data = ctx.get_control_event<d20::AcTransferLimits>()) {
            ctx.session_config.ac_limits = *control_data;
        } else if (const auto control_data = ctx.get_control_event<d20::UpdateDynamicModeParameters>()) {
            ctx.cache_dynamic_mode_parameters.emplace(*control_data);
        } else if (const auto control_data = ctx.get_control_event<d20::AcTargetPower>()) {
            ctx.cache_ac_target_power.emplace(*control_data);
        } else if (const auto control_data = ctx.get_control_event<d20::AcPresentPower>()) {
            ctx.cache_ac_present_power.emplace(*control_data);
        }
        // Save some control events. It can happen that these events are sent before the corresponding state. They are
        // stored temporarily here.
        // TODO(sl): Construct ControlEventCache Struct

        const auto control_res = fsm.feed(d20::Event::CONTROL_MESSAGE);
        // Control events are cached in Context (e.g., cache_dynamic_mode_parameters)
        // and picked up by the next state; unhandled is expected here.
    }

    const auto timeouts_reached = timeouts.check();

    if (timeouts_reached.has_value()) {
        const auto& reached = timeouts_reached.value();

        for (const auto& timeout : reached) {
            if (timeout == d20::TimeoutType::SEQUENCE) {
                logf_error("Sequence Timeout 40secs is reached. Stopping the session");
                ctx.session_stopped = true;
                break;
            } else {
                ctx.set_active_timeout(timeout);
                const auto timeout_res = fsm.feed(d20::Event::TIMEOUT);
                if (not timeout_res) {
                    logf_error("Timeout was not handled by current state. Stopping the session");
                    ctx.session_stopped = true;
                }
                timeouts.reset_timeout(timeout);
            }
        }
    }

    // check for complete sdp packet
    if (packet.is_complete()) {
        // FIXME (aw): this event loop only acts on new packets, seems to be enough for now ...
        log_packet_from_car(packet, log);

        message_exchange.set_request(make_variant_from_packet(packet));

        packet = {}; // reset the packet

        const auto request_msg_type = ctx.peek_request_type();

        // There is no sequence timer before SupportedAppProtocol
        if (request_msg_type != message_20::Type::SupportedAppProtocolReq) {
            timeouts.stop_timeout(d20::TimeoutType::SEQUENCE);
        }

        ctx.feedback.v2g_message(request_msg_type);

        const auto msg_res = fsm.feed(d20::Event::V2GTP_MESSAGE);
        if (not msg_res) {
            logf_error("V2GTP message was not handled by current state. Stopping the session");
            ctx.session_stopped = true;
            // Continue to check for pending response below.
        }
    }

    const auto [got_response, payload_size, payload_type, response_type] = message_exchange.check_and_clear_response();

    if (got_response) {
        const auto response_size = setup_response_header(response_buffer, payload_type, payload_size);
        connection->write(response_buffer, response_size);

        timeouts.start_timeout(d20::TimeoutType::SEQUENCE, d20::TIMEOUT_SEQUENCE);

        // FIXME (aw): this is hacky ...
        log.exi(static_cast<uint16_t>(payload_type), response_buffer + io::SdpPacket::V2GTP_HEADER_SIZE, payload_size,
                session::logging::ExiMessageDirection::TO_EV);

        ctx.feedback.v2g_message(response_type);
    }

    if (ctx.session_stopped or ctx.session_paused) {
        if (not delayed_close_at.has_value()) {
            // Schedule a non-blocking delayed close per [V2G20-1643].
            delayed_close_at = offset_time_point_by_ms(now, 5000);
        }

        if (now >= *delayed_close_at) {
            connection->close();
            delayed_close_at = std::nullopt;
            const auto signal =
                (ctx.session_paused) ? session::feedback::Signal::DLINK_PAUSE : session::feedback::Signal::DLINK_TERMINATE;
            ctx.feedback.signal(signal);
            next_session_event = offset_time_point_by_ms(now, SESSION_IDLE_TIMEOUT_MS);
            return next_session_event;
        }

        // Still waiting for the delayed-close window; tell the event loop
        // to wake us up at the scheduled close time.
        next_session_event = *delayed_close_at;
        return next_session_event;
    }

    next_session_event = offset_time_point_by_ms(now, SESSION_IDLE_TIMEOUT_MS);
    return next_session_event;
}

void Session::handle_connection_event(io::ConnectionEvent event) {
    using Event = io::ConnectionEvent;
    switch (event) {
    case Event::ACCEPTED:
        assert(state.connected == false);
        state.connected = true;
        // Public endpoint is available after accept.
        log("Accepted connection on port %d", connection->get_public_endpoint().port);
        return;

    case Event::NEW_DATA:
        assert(state.connected);
        state.new_data = true;
        return;

    case Event::OPEN:
        assert(state.connected);
        if (const auto new_vehicle_cert_hash = connection->get_vehicle_cert_hash()) {
            logf_info("Vehicle Cert is available");
            ctx.set_new_vehicle_cert_hash(new_vehicle_cert_hash);
        }
        // NOTE (aw): for now, we don't really need this information ...
        return;

    case Event::CLOSED:
        state.connected = false;
        logf_info("Connection is closed");
        return;
    }
}

void Session::close() {
    connection->close();
    ctx.feedback.signal(session::feedback::Signal::DLINK_TERMINATE);
    ctx.session_stopped = true;
}

} // namespace iso15118
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Pionix GmbH and Contributors to EVerest
#include <iso15118/d20/state/ac_charge_parameter_discovery.hpp>
#include <iso15118/d20/state/schedule_exchange.hpp>

#include <iso15118/detail/d20/context_helper.hpp>
#include <iso15118/detail/d20/state/ac_charge_parameter_discovery.hpp>
#include <iso15118/detail/d20/state/session_stop.hpp>
#include <iso15118/detail/helper.hpp>

namespace iso15118::d20::state {

namespace dt = message_20::datatypes;

using AC_ModeReq = dt::AC_CPDReqEnergyTransferMode;
using BPT_AC_ModeReq = dt::BPT_AC_CPDReqEnergyTransferMode;
using DER_AC_ModeReq = dt::DER_AC_CPDReqEnergyTransferMode;

using AC_ModeRes = dt::AC_CPDResEnergyTransferMode;
using BPT_AC_ModeRes = dt::BPT_AC_CPDResEnergyTransferMode;
using DER_AC_ModeRes = dt::DER_AC_CPDResEnergyTransferMode;

template <typename Out>
void convert(Out& out, const d20::AcTransferLimits& limits, const d20::AcPresentPower& present_power);

namespace {

bool has_mandatory_ac_der_controls(const dt::DERControlFunctions& controls) {
    return has_required_ac_der_control_functions(controls);
}

AcChargeParameterDiscoveryResult failed_ac_der_result(message_20::AC_ChargeParameterDiscoveryResponse& res,
                                                      const message_20::datatypes::ResponseCode response_code,
                                                      const AcDerControlFailureReason failure_reason) {
    return {response_with_code(res, response_code), failure_reason};
}

} // namespace

template <>
void convert(AC_ModeRes& out, const d20::AcTransferLimits& limits, const d20::AcPresentPower& present_power) {
    out.min_charge_power = limits.charge_power.min;
    out.max_charge_power = limits.charge_power.max;

    if (limits.charge_power_L2.has_value()) {
        out.min_charge_power_L2 = limits.charge_power_L2.value().min;
        out.max_charge_power_L2 = limits.charge_power_L2.value().max;
    }

    if (limits.charge_power_L3.has_value()) {
        out.min_charge_power_L3 = limits.charge_power_L3.value().min;
        out.max_charge_power_L3 = limits.charge_power_L3.value().max;
    }

    out.nominal_frequency = limits.nominal_frequency;
    out.max_power_asymmetry = limits.max_power_asymmetry;
    out.power_ramp_limitation = limits.power_ramp_limitation;
    out.present_active_power = present_power.present_active_power;
    out.present_active_power_L2 = present_power.present_active_power_L2;
    out.present_active_power_L3 = present_power.present_active_power_L3;
}

template <>
void convert(BPT_AC_ModeRes& out, const d20::AcTransferLimits& limits, const d20::AcPresentPower& present_power) {
    convert(static_cast<AC_ModeRes&>(out), limits, present_power);

    if (limits.discharge_power.has_value()) {
        out.min_discharge_power = limits.discharge_power.value().min;
        out.max_discharge_power = limits.discharge_power.value().max;
    }

    if (limits.discharge_power_L2.has_value()) {
        out.min_discharge_power_L2 = limits.discharge_power_L2.value().min;
        out.max_discharge_power_L2 = limits.discharge_power_L2.value().max;
    }

    if (limits.discharge_power_L3.has_value()) {
        out.min_discharge_power_L3 = limits.discharge_power_L3.value().min;
        out.max_discharge_power_L3 = limits.discharge_power_L3.value().max;
    }
}

template <>
void convert(DER_AC_ModeRes& out, const d20::AcTransferLimits& limits, const d20::AcPresentPower& present_power) {
    convert(static_cast<AC_ModeRes&>(out), limits, present_power);

    out.nominal_charge_power = limits.charge_power.max;
    if (limits.charge_power_L2.has_value()) {
        out.nominal_charge_power_L2 = limits.charge_power_L2.value().max;
    }
    if (limits.charge_power_L3.has_value()) {
        out.nominal_charge_power_L3 = limits.charge_power_L3.value().max;
    }

    if (limits.discharge_power.has_value()) {
        out.nominal_discharge_power = limits.discharge_power.value().max;
        out.max_discharge_power = limits.discharge_power.value().max;
    } else {
        out.nominal_discharge_power = dt::RationalNumber{0, 0};
        out.max_discharge_power = dt::RationalNumber{0, 0};
    }
    if (limits.discharge_power_L2.has_value()) {
        out.nominal_discharge_power_L2 = limits.discharge_power_L2.value().max;
        out.max_discharge_power_L2 = limits.discharge_power_L2.value().max;
    }
    if (limits.discharge_power_L3.has_value()) {
        out.nominal_discharge_power_L3 = limits.discharge_power_L3.value().max;
        out.max_discharge_power_L3 = limits.discharge_power_L3.value().max;
    }

    out.operating_mode = dt::EVOperatingMode::GridFollowing;
    out.grid_connection_mode = dt::GridConnectionMode::GridConnected;
}

AcChargeParameterDiscoveryResult
handle_request_with_diagnostics(const message_20::AC_ChargeParameterDiscoveryRequest& req, const d20::Session& session,
                                const d20::AcTransferLimits& limits, const d20::AcPresentPower& powers,
                                const d20::IAcDerControlProvider& ac_der_control_provider) {

    message_20::AC_ChargeParameterDiscoveryResponse res;

    if (validate_and_setup_header(res.header, session, req.header.session_id) == false) {
        return {response_with_code(res, message_20::datatypes::ResponseCode::FAILED_UnknownSession),
                AcDerControlFailureReason::None};
    }

    const auto selected_services = session.get_selected_services();
    const auto selected_energy_service = selected_services.selected_energy_service;
    const auto provider_context = AcDerControlContext{selected_energy_service, selected_services.selected_control_mode,
                                                      selected_services.selected_mobility_needs_mode,
                                                      selected_services.selected_der_control_functions};

    if (std::holds_alternative<AC_ModeReq>(req.transfer_mode)) {
        if (selected_energy_service != message_20::datatypes::ServiceCategory::AC) {
            return {response_with_code(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter),
                    AcDerControlFailureReason::None};
        }

        auto& mode = res.transfer_mode.emplace<AC_ModeRes>();
        convert(mode, limits, powers);

    } else if (std::holds_alternative<BPT_AC_ModeReq>(req.transfer_mode)) {
        if (selected_energy_service != message_20::datatypes::ServiceCategory::AC_BPT) {
            return {response_with_code(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter),
                    AcDerControlFailureReason::None};
        }

        auto& mode = res.transfer_mode.emplace<BPT_AC_ModeRes>();
        convert(mode, limits, powers);

    } else if (std::holds_alternative<DER_AC_ModeReq>(req.transfer_mode)) {
        if (selected_energy_service != message_20::datatypes::ServiceCategory::AC_DER &&
            selected_energy_service != message_20::datatypes::ServiceCategory::AC_BPT) {
            return failed_ac_der_result(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter,
                                        AcDerControlFailureReason::NonAcDerServiceSelected);
        }
        if (not selected_services.selected_der_control_functions.has_value()) {
            return failed_ac_der_result(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter,
                                        AcDerControlFailureReason::MissingSelectedControlFunctions);
        }
        const auto& controls = selected_services.selected_der_control_functions.value();
        if (not has_mandatory_ac_der_controls(controls)) {
            return failed_ac_der_result(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter,
                                        AcDerControlFailureReason::MissingSelectedMandatoryControlFunctions);
        }
        const auto ac_der_control_result = ac_der_control_provider.get_ac_der_control_result(provider_context);
        if (not ac_der_control_result.config.has_value()) {
            return failed_ac_der_result(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter,
                                        ac_der_control_result.failure_reason);
        }
        if (not validate_ac_der_control_config(ac_der_control_result.config.value(), controls)) {
            return failed_ac_der_result(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter,
                                        AcDerControlFailureReason::InvalidDsoControl);
        }

        auto& mode = res.transfer_mode.emplace<DER_AC_ModeRes>();
        convert(mode, limits, powers);
        mode.der_control = ac_der_control_result.config->cpd_control;

    } else {
        return {response_with_code(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter),
                AcDerControlFailureReason::None};
    }

    return {response_with_code(res, message_20::datatypes::ResponseCode::OK), AcDerControlFailureReason::None};
}

message_20::AC_ChargeParameterDiscoveryResponse
handle_request(const message_20::AC_ChargeParameterDiscoveryRequest& req, const d20::Session& session,
               const d20::AcTransferLimits& limits, const d20::AcPresentPower& powers,
               const d20::IAcDerControlProvider& ac_der_control_provider) {
    return handle_request_with_diagnostics(req, session, limits, powers, ac_der_control_provider).response;
}

message_20::AC_ChargeParameterDiscoveryResponse
handle_request(const message_20::AC_ChargeParameterDiscoveryRequest& req, const d20::Session& session,
               const d20::AcTransferLimits& limits, const d20::AcPresentPower& powers,
               const d20::AcDerControlConfig& ac_der_control_config) {
    return handle_request(req, session, limits, powers, *make_static_ac_der_control_provider(ac_der_control_config));
}

message_20::AC_ChargeParameterDiscoveryResponse
handle_request(const message_20::AC_ChargeParameterDiscoveryRequest& req, const d20::Session& session,
               const d20::AcTransferLimits& limits, const d20::AcPresentPower& powers) {
    return handle_request(req, session, limits, powers,
                          *make_static_ac_der_control_provider(make_default_ac_der_control_config()));
}

void AC_ChargeParameterDiscovery::enter() {
    m_ctx.log.enter_state("AC_ChargeParameterDiscovery");
    present_powers = m_ctx.cache_ac_present_power.value_or(AcPresentPower{});
}

Result AC_ChargeParameterDiscovery::feed(Event ev) {

    if (ev == Event::CONTROL_MESSAGE) {
        if (const auto* control_data = m_ctx.get_control_event<AcPresentPower>()) {
            present_powers = *control_data;
        }
        return {};
    }

    if (ev != Event::V2GTP_MESSAGE) {
        return {};
    }

    const auto variant = m_ctx.pull_request();

    if (const auto req = variant->get_if<message_20::AC_ChargeParameterDiscoveryRequest>()) {

        if (const auto* mode = std::get_if<AC_ModeReq>(&req->transfer_mode)) {
            // Set EV transfer limits
            m_ctx.session_ev_info.ev_transfer_limits.emplace<AC_ModeReq>(*mode);
        } else if (const auto* mode = std::get_if<BPT_AC_ModeReq>(&req->transfer_mode)) {
            // Set EV transfer limits
            m_ctx.session_ev_info.ev_transfer_limits.emplace<BPT_AC_ModeReq>(*mode);
        } else if (const auto* mode = std::get_if<DER_AC_ModeReq>(&req->transfer_mode)) {
            // Set EV transfer limits
            m_ctx.session_ev_info.ev_transfer_limits.emplace<DER_AC_ModeReq>(*mode);
        }

        const auto result =
            handle_request_with_diagnostics(*req, m_ctx.session, m_ctx.session_config.ac_limits, present_powers,
                                            *m_ctx.session_config.ac_der_control_provider);
        const auto& res = result.response;

        m_ctx.respond(res);

        if (res.response_code >= message_20::datatypes::ResponseCode::FAILED) {
            if (result.ac_der_failure_reason != AcDerControlFailureReason::None) {
                m_ctx.feedback.ac_der_control_diagnostic({message_20::Type::AC_ChargeParameterDiscoveryReq,
                                                          res.response_code, result.ac_der_failure_reason});
                m_ctx.log("AC DER control provider rejected CPD request: %s",
                          ac_der_control_failure_reason_to_string(result.ac_der_failure_reason));
            }
            m_ctx.session_stopped = true;
            return {};
        }

        m_ctx.feedback.ac_limits(req->transfer_mode);

        return m_ctx.create_state<ScheduleExchange>();

    } else if (const auto req = variant->get_if<message_20::SessionStopRequest>()) {
        const auto res = handle_request(*req, m_ctx.session);

        m_ctx.respond(res);
        m_ctx.session_stopped = true;

        return {};
    } else {
        m_ctx.log("expected AC_ChargeParameterDiscovery! But code type id: %d", variant->get_type());
        m_ctx.session_stopped = true;

        // Sequence Error
        const message_20::Type req_type = variant->get_type();
        send_sequence_error(req_type, m_ctx);

        return {};
    }
}

} // namespace iso15118::d20::state

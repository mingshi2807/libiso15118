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

dt::RationalNumber zero() {
    return {0, 0};
}

dt::RationalNumber one() {
    return {1, 0};
}

dt::RationalNumber seconds(const uint16_t value) {
    return {static_cast<int16_t>(value), 0};
}

bool has_mandatory_ac_der_controls(const dt::DERControlFunctions& controls) {
    return controls.volt_watt and controls.dso_q_setpoint_provision and controls.dso_cos_phi_setpoint_provision and
           controls.dc_injection_restriction and controls.under_frequency_watt and controls.over_frequency_watt and
           controls.volt_var and controls.watt_var and controls.watt_cos_phi and
           controls.over_voltage_fault_ride_through and controls.zero_current;
}

dt::DERCurve make_der_curve(const dt::DERCurveDataUnit x_unit, const dt::DERCurveDataUnit y_unit) {
    dt::DERCurve curve;
    curve.x_unit = x_unit;
    curve.y_unit = y_unit;
    curve.curve_data_points.push_back({zero(), {zero(), std::nullopt}});
    curve.curve_data_points.push_back({one(), {one(), std::nullopt}});
    curve.pt1_response_reactive_power = false;
    curve.step_response_time_constant_reactive_power = seconds(0);
    return curve;
}

dt::FrequencyWatt make_frequency_watt(const float start_frequency, const float stop_frequency) {
    dt::FrequencyWatt frequency_watt;
    frequency_watt.f_start = dt::from_float(start_frequency);
    frequency_watt.f_stop = dt::from_float(stop_frequency);
    frequency_watt.slope = one();
    frequency_watt.power_reference = dt::DERPowerReference::MaximumDischargePower;
    frequency_watt.hysteresis_control = false;
    frequency_watt.pt1_response_active_power = false;
    frequency_watt.step_response_time_constant_active_power = seconds(0);
    return frequency_watt;
}

dt::VoltWatt make_volt_watt() {
    dt::VoltWatt volt_watt;
    volt_watt.power_reference = dt::DERPowerReference::MaximumDischargePower;
    volt_watt.u_start = dt::from_float(230.0f);
    volt_watt.u_stop = dt::from_float(253.0f);
    volt_watt.pt1_response_active_power = false;
    volt_watt.step_response_time_constant_active_power = seconds(0);
    return volt_watt;
}

dt::FaultRideThrough make_fault_ride_through() {
    dt::FaultRideThrough fault_ride_through;
    fault_ride_through.voltage_limit_start_frt = dt::from_float(253.0f);
    fault_ride_through.pt1_response_active_power = false;
    fault_ride_through.step_response_time_constant_active_power = seconds(0);
    fault_ride_through.pt1_response_reactive_power = false;
    fault_ride_through.step_response_time_constant_reactive_power = seconds(0);
    return fault_ride_through;
}

dt::ZeroCurrent make_zero_current() {
    dt::ZeroCurrent zero_current;
    zero_current.over_voltage_limit = dt::from_float(253.0f);
    zero_current.pt1_response_active_power = false;
    zero_current.step_response_time_constant_active_power = seconds(0);
    zero_current.pt1_response_reactive_power = false;
    zero_current.step_response_time_constant_reactive_power = seconds(0);
    return zero_current;
}

dt::DERControl make_der_control(const dt::DERControlFunctions& controls) {
    dt::DERControl der_control;

    if (controls.over_voltage_fault_ride_through) {
        der_control.overvoltage_fault_ride_through = make_fault_ride_through();
    }

    if (controls.under_voltage_fault_ride_through) {
        der_control.undervoltage_fault_ride_through = make_fault_ride_through();
    }

    if (controls.zero_current) {
        der_control.zero_current = make_zero_current();
    }

    if (controls.volt_var or controls.watt_var or controls.watt_cos_phi) {
        auto& reactive_power_support = der_control.reactive_power_support.emplace();
        if (controls.volt_var) {
            reactive_power_support.volt_var = make_der_curve(dt::DERCurveDataUnit::V, dt::DERCurveDataUnit::var);
        }
        if (controls.watt_var) {
            reactive_power_support.watt_var = make_der_curve(dt::DERCurveDataUnit::W, dt::DERCurveDataUnit::var);
        }
        if (controls.watt_cos_phi) {
            reactive_power_support.watt_cos_phi = make_der_curve(dt::DERCurveDataUnit::W, dt::DERCurveDataUnit::var);
            reactive_power_support.watt_cos_phi->curve_data_points[0].y_value.excitation =
                dt::DERPowerFactorExcitation::OverExcited;
            reactive_power_support.watt_cos_phi->curve_data_points[1].y_value.excitation =
                dt::DERPowerFactorExcitation::UnderExcited;
        }
    }

    if (controls.under_frequency_watt or controls.over_frequency_watt or controls.volt_watt) {
        auto& active_power_support = der_control.active_power_support.emplace();
        if (controls.under_frequency_watt) {
            active_power_support.under_frequency_watt = make_frequency_watt(49.8f, 49.5f);
        }
        if (controls.over_frequency_watt) {
            active_power_support.over_frequency_watt = make_frequency_watt(50.2f, 50.5f);
        }
        if (controls.volt_watt) {
            active_power_support.volt_watt = make_volt_watt();
        }
    }

    if (controls.dc_injection_restriction) {
        der_control.maximum_level_dc_injection = zero();
    }

    return der_control;
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

message_20::AC_ChargeParameterDiscoveryResponse
handle_request(const message_20::AC_ChargeParameterDiscoveryRequest& req, const d20::Session& session,
               const d20::AcTransferLimits& limits, const d20::AcPresentPower& powers) {

    message_20::AC_ChargeParameterDiscoveryResponse res;

    if (validate_and_setup_header(res.header, session, req.header.session_id) == false) {
        return response_with_code(res, message_20::datatypes::ResponseCode::FAILED_UnknownSession);
    }

    const auto selected_services = session.get_selected_services();
    const auto selected_energy_service = selected_services.selected_energy_service;

    if (std::holds_alternative<AC_ModeReq>(req.transfer_mode)) {
        if (selected_energy_service != message_20::datatypes::ServiceCategory::AC) {
            return response_with_code(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter);
        }

        auto& mode = res.transfer_mode.emplace<AC_ModeRes>();
        convert(mode, limits, powers);

    } else if (std::holds_alternative<BPT_AC_ModeReq>(req.transfer_mode)) {
        if (selected_energy_service != message_20::datatypes::ServiceCategory::AC_BPT) {
            return response_with_code(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter);
        }

        auto& mode = res.transfer_mode.emplace<BPT_AC_ModeRes>();
        convert(mode, limits, powers);

    } else if (std::holds_alternative<DER_AC_ModeReq>(req.transfer_mode)) {
        if (selected_energy_service != message_20::datatypes::ServiceCategory::AC_DER) {
            return response_with_code(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter);
        }
        if (not selected_services.selected_der_control_functions.has_value() or
            not has_mandatory_ac_der_controls(selected_services.selected_der_control_functions.value())) {
            return response_with_code(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter);
        }

        auto& mode = res.transfer_mode.emplace<DER_AC_ModeRes>();
        convert(mode, limits, powers);
        mode.der_control = make_der_control(selected_services.selected_der_control_functions.value());

    } else {
        return response_with_code(res, message_20::datatypes::ResponseCode::FAILED_WrongChargeParameter);
    }

    return response_with_code(res, message_20::datatypes::ResponseCode::OK);
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

        const auto res = handle_request(*req, m_ctx.session, m_ctx.session_config.ac_limits, present_powers);

        m_ctx.respond(res);

        if (res.response_code >= message_20::datatypes::ResponseCode::FAILED) {
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

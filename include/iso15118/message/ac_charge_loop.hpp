// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Pionix GmbH and Contributors to EVerest
#pragma once

#include <variant>
#include <vector>

#include "common_types.hpp"

namespace iso15118::message_20 {

namespace datatypes {

struct Scheduled_AC_CLReqControlMode : Scheduled_CLReqControlMode {
    std::optional<RationalNumber> max_charge_power;
    std::optional<RationalNumber> max_charge_power_L2;
    std::optional<RationalNumber> max_charge_power_L3;
    std::optional<RationalNumber> min_charge_power;
    std::optional<RationalNumber> min_charge_power_L2;
    std::optional<RationalNumber> min_charge_power_L3;
    RationalNumber present_active_power;
    std::optional<RationalNumber> present_active_power_L2;
    std::optional<RationalNumber> present_active_power_L3;
    std::optional<RationalNumber> present_reactive_power;
    std::optional<RationalNumber> present_reactive_power_L2;
    std::optional<RationalNumber> present_reactive_power_L3;
};

struct BPT_Scheduled_AC_CLReqControlMode : Scheduled_AC_CLReqControlMode {
    std::optional<RationalNumber> max_discharge_power;
    std::optional<RationalNumber> max_discharge_power_L2;
    std::optional<RationalNumber> max_discharge_power_L3;
    std::optional<RationalNumber> min_discharge_power;
    std::optional<RationalNumber> min_discharge_power_L2;
    std::optional<RationalNumber> min_discharge_power_L3;
};

struct DER_Scheduled_AC_CLReqControlMode : BPT_Scheduled_AC_CLReqControlMode {
    std::optional<RationalNumber> max_charge_reactive_power;
    std::optional<RationalNumber> max_charge_reactive_power_L2;
    std::optional<RationalNumber> max_charge_reactive_power_L3;
    std::optional<RationalNumber> max_discharge_reactive_power;
    std::optional<RationalNumber> max_discharge_reactive_power_L2;
    std::optional<RationalNumber> max_discharge_reactive_power_L3;
    uint8_t grid_event_condition{0};
};

struct Dynamic_AC_CLReqControlMode : Dynamic_CLReqControlMode {
    RationalNumber max_charge_power;
    std::optional<RationalNumber> max_charge_power_L2;
    std::optional<RationalNumber> max_charge_power_L3;
    RationalNumber min_charge_power;
    std::optional<RationalNumber> min_charge_power_L2;
    std::optional<RationalNumber> min_charge_power_L3;
    RationalNumber present_active_power;
    std::optional<RationalNumber> present_active_power_L2;
    std::optional<RationalNumber> present_active_power_L3;
    RationalNumber present_reactive_power;
    std::optional<RationalNumber> present_reactive_power_L2;
    std::optional<RationalNumber> present_reactive_power_L3;
};

struct BPT_Dynamic_AC_CLReqControlMode : Dynamic_AC_CLReqControlMode {
    RationalNumber max_discharge_power;
    std::optional<RationalNumber> max_discharge_power_L2;
    std::optional<RationalNumber> max_discharge_power_L3;
    RationalNumber min_discharge_power;
    std::optional<RationalNumber> min_discharge_power_L2;
    std::optional<RationalNumber> min_discharge_power_L3;
    std::optional<RationalNumber> max_v2x_energy_request;
    std::optional<RationalNumber> min_v2x_energy_request;
};

struct DER_Dynamic_AC_CLReqControlMode : BPT_Dynamic_AC_CLReqControlMode {
    std::optional<RationalNumber> max_charge_reactive_power;
    std::optional<RationalNumber> max_charge_reactive_power_L2;
    std::optional<RationalNumber> max_charge_reactive_power_L3;
    std::optional<RationalNumber> max_discharge_reactive_power;
    std::optional<RationalNumber> max_discharge_reactive_power_L2;
    std::optional<RationalNumber> max_discharge_reactive_power_L3;
    uint8_t grid_event_condition{0};
    std::optional<RationalNumber> session_total_discharge_energy_available;
};

struct Scheduled_AC_CLResControlMode : Scheduled_CLResControlMode {
    std::optional<RationalNumber> target_active_power;
    std::optional<RationalNumber> target_active_power_L2;
    std::optional<RationalNumber> target_active_power_L3;
    std::optional<RationalNumber> target_reactive_power;
    std::optional<RationalNumber> target_reactive_power_L2;
    std::optional<RationalNumber> target_reactive_power_L3;
    std::optional<RationalNumber> present_active_power;
    std::optional<RationalNumber> present_active_power_L2;
    std::optional<RationalNumber> present_active_power_L3;
};

struct BPT_Scheduled_AC_CLResControlMode : Scheduled_AC_CLResControlMode {};

struct DSOQSetpoint {
    RationalNumber value;
    std::optional<RationalNumber> value_L2;
    std::optional<RationalNumber> value_L3;
    bool pt1_response_reactive_power;
    RationalNumber step_response_time_constant_reactive_power;
};

struct DSOCosPhiSetpoint {
    RationalNumber value;
    std::optional<RationalNumber> value_L2;
    std::optional<RationalNumber> value_L3;
    DERPowerFactorExcitation excitation;
    bool pt1_response_reactive_power;
    RationalNumber step_response_time_constant_reactive_power;
};

struct DER_Scheduled_AC_CLResControlMode : Scheduled_AC_CLResControlMode {
    RationalNumber max_charge_power;
    std::optional<RationalNumber> max_charge_power_L2;
    std::optional<RationalNumber> max_charge_power_L3;
    RationalNumber max_discharge_power;
    std::optional<RationalNumber> max_discharge_power_L2;
    std::optional<RationalNumber> max_discharge_power_L3;
    std::optional<RationalNumber> dso_max_discharge_power;
    std::optional<RationalNumber> dso_max_discharge_power_L2;
    std::optional<RationalNumber> dso_max_discharge_power_L3;
    std::optional<DSOQSetpoint> dso_q_setpoint;
    std::optional<DSOCosPhiSetpoint> dso_cos_phi_setpoint;
};

struct Dynamic_AC_CLResControlMode : Dynamic_CLResControlMode {
    RationalNumber target_active_power;
    std::optional<RationalNumber> target_active_power_L2;
    std::optional<RationalNumber> target_active_power_L3;
    std::optional<RationalNumber> target_reactive_power;
    std::optional<RationalNumber> target_reactive_power_L2;
    std::optional<RationalNumber> target_reactive_power_L3;
    std::optional<RationalNumber> present_active_power;
    std::optional<RationalNumber> present_active_power_L2;
    std::optional<RationalNumber> present_active_power_L3;
};

struct BPT_Dynamic_AC_CLResControlMode : Dynamic_AC_CLResControlMode {};

struct DER_Dynamic_AC_CLResControlMode : Dynamic_AC_CLResControlMode {
    RationalNumber max_charge_power;
    std::optional<RationalNumber> max_charge_power_L2;
    std::optional<RationalNumber> max_charge_power_L3;
    RationalNumber max_discharge_power;
    std::optional<RationalNumber> max_discharge_power_L2;
    std::optional<RationalNumber> max_discharge_power_L3;
    std::optional<RationalNumber> dso_max_discharge_power;
    std::optional<RationalNumber> dso_max_discharge_power_L2;
    std::optional<RationalNumber> dso_max_discharge_power_L3;
    std::optional<DSOQSetpoint> dso_q_setpoint;
    std::optional<DSOCosPhiSetpoint> dso_cos_phi_setpoint;
};

} // namespace datatypes

struct AC_ChargeLoopRequest {
    Header header;

    // the following 2 are inherited from ChargeLoopReq
    std::optional<datatypes::DisplayParameters> display_parameters;
    bool meter_info_requested;

    std::variant<datatypes::Scheduled_AC_CLReqControlMode, datatypes::BPT_Scheduled_AC_CLReqControlMode,
                 datatypes::DER_Scheduled_AC_CLReqControlMode, datatypes::Dynamic_AC_CLReqControlMode,
                 datatypes::BPT_Dynamic_AC_CLReqControlMode, datatypes::DER_Dynamic_AC_CLReqControlMode>
        control_mode;
};

struct AC_ChargeLoopResponse {
    Header header;
    datatypes::ResponseCode response_code;

    // the following 3 are inherited from ChargeLoopRes
    std::optional<datatypes::EvseStatus> status;
    std::optional<datatypes::MeterInfo> meter_info;
    std::optional<datatypes::Receipt> receipt;

    std::optional<datatypes::RationalNumber> target_frequency;

    std::variant<datatypes::Scheduled_AC_CLResControlMode, datatypes::BPT_Scheduled_AC_CLResControlMode,
                 datatypes::DER_Scheduled_AC_CLResControlMode, datatypes::Dynamic_AC_CLResControlMode,
                 datatypes::BPT_Dynamic_AC_CLResControlMode, datatypes::DER_Dynamic_AC_CLResControlMode>
        control_mode = datatypes::Scheduled_AC_CLResControlMode();
};

} // namespace iso15118::message_20

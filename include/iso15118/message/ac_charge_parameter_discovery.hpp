// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Pionix GmbH and Contributors to EVerest
#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "common_types.hpp"

namespace iso15118::message_20 {

namespace datatypes {

struct AC_CPDReqEnergyTransferMode {
    RationalNumber max_charge_power;
    std::optional<RationalNumber> max_charge_power_L2;
    std::optional<RationalNumber> max_charge_power_L3;
    RationalNumber min_charge_power;
    std::optional<RationalNumber> min_charge_power_L2;
    std::optional<RationalNumber> min_charge_power_L3;
};

struct BPT_AC_CPDReqEnergyTransferMode : AC_CPDReqEnergyTransferMode {
    RationalNumber max_discharge_power;
    std::optional<RationalNumber> max_discharge_power_L2;
    std::optional<RationalNumber> max_discharge_power_L3;
    RationalNumber min_discharge_power;
    std::optional<RationalNumber> min_discharge_power_L2;
    std::optional<RationalNumber> min_discharge_power_L3;
};

struct EVReactivePowerLimits {
    RationalNumber max_charge_reactive_power;
    std::optional<RationalNumber> max_charge_reactive_power_L2;
    std::optional<RationalNumber> max_charge_reactive_power_L3;
    std::optional<RationalNumber> min_charge_reactive_power;
    std::optional<RationalNumber> min_charge_reactive_power_L2;
    std::optional<RationalNumber> min_charge_reactive_power_L3;
    RationalNumber max_discharge_reactive_power;
    std::optional<RationalNumber> max_discharge_reactive_power_L2;
    std::optional<RationalNumber> max_discharge_reactive_power_L3;
    std::optional<RationalNumber> min_discharge_reactive_power;
    std::optional<RationalNumber> min_discharge_reactive_power_L2;
    std::optional<RationalNumber> min_discharge_reactive_power_L3;
};

struct DER_AC_CPDReqEnergyTransferMode : BPT_AC_CPDReqEnergyTransferMode {
    Processing processing;
    std::optional<RationalNumber> session_total_discharge_energy_available;
    std::optional<EVReactivePowerLimits> reactive_power_limits;
};

struct AC_CPDResEnergyTransferMode {
    RationalNumber max_charge_power;
    std::optional<RationalNumber> max_charge_power_L2;
    std::optional<RationalNumber> max_charge_power_L3;
    RationalNumber min_charge_power;
    std::optional<RationalNumber> min_charge_power_L2;
    std::optional<RationalNumber> min_charge_power_L3;
    RationalNumber nominal_frequency;
    std::optional<RationalNumber> max_power_asymmetry;
    std::optional<RationalNumber> power_ramp_limitation;
    std::optional<RationalNumber> present_active_power;
    std::optional<RationalNumber> present_active_power_L2;
    std::optional<RationalNumber> present_active_power_L3;
};

struct BPT_AC_CPDResEnergyTransferMode : AC_CPDResEnergyTransferMode {
    RationalNumber max_discharge_power;
    std::optional<RationalNumber> max_discharge_power_L2;
    std::optional<RationalNumber> max_discharge_power_L3;
    RationalNumber min_discharge_power;
    std::optional<RationalNumber> min_discharge_power_L2;
    std::optional<RationalNumber> min_discharge_power_L3;
};

enum class EVOperatingMode {
    GridFollowing = 0,
    GridForming = 1,
};

enum class GridConnectionMode {
    GridConnected = 0,
    GridIslanded = 1,
};

struct DERSetpointExcitation {
    RationalNumber setpoint_value;
    std::optional<DERPowerFactorExcitation> excitation;
};

struct DERCurveDataPoint {
    RationalNumber x_value;
    DERSetpointExcitation y_value;
};

struct DERCurve {
    DERCurveDataUnit x_unit;
    DERCurveDataUnit y_unit;
    std::vector<DERCurveDataPoint> curve_data_points;
    std::optional<RationalNumber> min_cos_phi;
    std::optional<DERCurveDataUnit> lock_value_unit;
    std::optional<RationalNumber> lock_in_value;
    std::optional<RationalNumber> lock_out_value;
    bool pt1_response_reactive_power{false};
    RationalNumber step_response_time_constant_reactive_power;
    std::optional<RationalNumber> intentional_delay;
};

struct FrequencyWatt {
    RationalNumber f_start;
    RationalNumber f_stop;
    std::optional<uint16_t> intentional_delay_f_stop;
    RationalNumber slope;
    std::optional<uint16_t> deactivation_time;
    std::optional<uint16_t> intentional_delay_power_control;
    DERPowerReference power_reference;
    bool hysteresis_control{false};
    std::optional<uint16_t> power_up_ramp;
    bool pt1_response_active_power{false};
    RationalNumber step_response_time_constant_active_power;
};

struct VoltWatt {
    DERPowerReference power_reference;
    RationalNumber u_start;
    RationalNumber u_stop;
    bool pt1_response_active_power{false};
    RationalNumber step_response_time_constant_active_power;
    std::optional<uint32_t> intentional_delay_power_control;
};

struct FaultRideThrough {
    RationalNumber voltage_limit_start_frt;
    std::optional<RationalNumber> voltage_limit_stop_frt;
    std::optional<RationalNumber> voltage_recovery_limit;
    std::optional<RationalNumber> voltage_ride_through_positive_curve_k_factor;
    std::optional<RationalNumber> voltage_ride_through_negative_curve_k_factor;
    bool pt1_response_active_power{false};
    RationalNumber step_response_time_constant_active_power;
    bool pt1_response_reactive_power{false};
    RationalNumber step_response_time_constant_reactive_power;
};

struct ZeroCurrent {
    std::optional<RationalNumber> over_voltage_limit;
    std::optional<RationalNumber> under_voltage_limit;
    std::optional<RationalNumber> over_voltage_recovery_limit;
    std::optional<RationalNumber> under_voltage_recovery_limit;
    bool pt1_response_active_power{false};
    RationalNumber step_response_time_constant_active_power;
    bool pt1_response_reactive_power{false};
    RationalNumber step_response_time_constant_reactive_power;
};

struct ReactivePowerSupport {
    std::optional<DERCurve> volt_var;
    std::optional<DERCurve> watt_var;
    std::optional<DERCurve> watt_cos_phi;
};

struct ActivePowerSupport {
    std::optional<FrequencyWatt> under_frequency_watt;
    std::optional<FrequencyWatt> over_frequency_watt;
    std::optional<VoltWatt> volt_watt;
};

struct DERControl {
    std::optional<FaultRideThrough> overvoltage_fault_ride_through;
    std::optional<FaultRideThrough> undervoltage_fault_ride_through;
    std::optional<ZeroCurrent> zero_current;
    std::optional<ReactivePowerSupport> reactive_power_support;
    std::optional<ActivePowerSupport> active_power_support;
    std::optional<RationalNumber> maximum_level_dc_injection;
};

struct DER_AC_CPDResEnergyTransferMode : AC_CPDResEnergyTransferMode {
    RationalNumber nominal_charge_power;
    std::optional<RationalNumber> nominal_charge_power_L2;
    std::optional<RationalNumber> nominal_charge_power_L3;
    RationalNumber nominal_discharge_power;
    std::optional<RationalNumber> nominal_discharge_power_L2;
    std::optional<RationalNumber> nominal_discharge_power_L3;
    RationalNumber max_discharge_power;
    std::optional<RationalNumber> max_discharge_power_L2;
    std::optional<RationalNumber> max_discharge_power_L3;
    EVOperatingMode operating_mode;
    GridConnectionMode grid_connection_mode;
    DERControl der_control;
};

} // namespace datatypes

struct AC_ChargeParameterDiscoveryRequest {
    Header header;
    std::variant<datatypes::AC_CPDReqEnergyTransferMode, datatypes::BPT_AC_CPDReqEnergyTransferMode,
                 datatypes::DER_AC_CPDReqEnergyTransferMode>
        transfer_mode;
};

struct AC_ChargeParameterDiscoveryResponse {
    Header header;
    datatypes::ResponseCode response_code;

    std::variant<datatypes::AC_CPDResEnergyTransferMode, datatypes::BPT_AC_CPDResEnergyTransferMode,
                 datatypes::DER_AC_CPDResEnergyTransferMode>
        transfer_mode = datatypes::AC_CPDResEnergyTransferMode();
};

} // namespace iso15118::message_20

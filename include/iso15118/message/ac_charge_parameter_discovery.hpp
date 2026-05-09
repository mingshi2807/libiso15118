// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Pionix GmbH and Contributors to EVerest
#pragma once

#include <optional>
#include <variant>

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

struct DERControl {
    // Placeholder for the detailed DER control function model. The generated
    // codec already carries this mandatory AC_DER_IEC field.
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

// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pionix GmbH and Contributors to EVerest
#pragma once

#include <optional>

#include <iso15118/message/common_types.hpp>

namespace iso15118::d20 {

namespace dt = message_20::datatypes;

// AC-side power setpoints sent by the EVSE during AC charge loop (ISO 15118-20).
// Each field is optional because the requested control mode may not require it.
// Values use RationalNumber (value * 10^exponent) from message/common_types.hpp.
struct AcTargetPower {
    // Aggregate/phase-agnostic active power setpoint (W).
    std::optional<dt::RationalNumber> target_active_power;
    // Per-phase active power setpoints (W) for L2/L3 (L1 uses target_active_power).
    std::optional<dt::RationalNumber> target_active_power_L2;
    std::optional<dt::RationalNumber> target_active_power_L3;
    // Aggregate/phase-agnostic reactive power setpoint (var).
    std::optional<dt::RationalNumber> target_reactive_power;
    // Per-phase reactive power setpoints (var) for L2/L3 (L1 uses target_reactive_power).
    std::optional<dt::RationalNumber> target_reactive_power_L2;
    std::optional<dt::RationalNumber> target_reactive_power_L3;
    // Target line frequency setpoint (Hz) when the EVSE provides it.
    std::optional<dt::RationalNumber> target_frequency;
};

// AC-side measured/feedback power values reported by the EVSE.
// Optional because the EVSE may only report aggregate or per-phase values.
struct AcPresentPower {
    // Aggregate/phase-agnostic present active power (W).
    std::optional<dt::RationalNumber> present_active_power;
    // Per-phase present active power (W) for L2/L3 (L1 uses present_active_power).
    std::optional<dt::RationalNumber> present_active_power_L2;
    std::optional<dt::RationalNumber> present_active_power_L3;
};

} // namespace iso15118::d20

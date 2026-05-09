// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Pionix GmbH and Contributors to EVerest
#pragma once

#include <iso15118/message/ac_charge_loop.hpp>
#include <iso15118/message/ac_charge_parameter_discovery.hpp>

namespace iso15118::d20 {

namespace dt = message_20::datatypes;

struct AcDerControlConfig {
    dt::DERControl cpd_control;
    dt::DSOQSetpoint dso_q_setpoint;
    dt::DSOCosPhiSetpoint dso_cos_phi_setpoint;
};

AcDerControlConfig make_default_ac_der_control_config();

} // namespace iso15118::d20

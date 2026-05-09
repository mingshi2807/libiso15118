// SPDX-License-Identifier: Apache-2.0
// Vedecom 2026 : Contributors to EVerest
#pragma once

#include <memory>
#include <optional>

#include <iso15118/message/ac_charge_loop.hpp>
#include <iso15118/message/ac_charge_parameter_discovery.hpp>
#include <iso15118/message/common_types.hpp>

namespace iso15118::d20 {

namespace dt = message_20::datatypes;

struct AcDerControlConfig {
    dt::DERControl cpd_control;
    dt::DSOQSetpoint dso_q_setpoint;
    dt::DSOCosPhiSetpoint dso_cos_phi_setpoint;
};

struct AcDerControlContext {
    dt::ServiceCategory selected_energy_service;
    dt::ControlMode selected_control_mode;
    dt::MobilityNeedsMode selected_mobility_needs_mode;
    std::optional<dt::DERControlFunctions> selected_der_control_functions;
};

class IAcDerControlProvider {
public:
    virtual ~IAcDerControlProvider() = default;

    virtual std::optional<AcDerControlConfig> get_ac_der_control_config(const AcDerControlContext& context) const = 0;
};

AcDerControlConfig make_default_ac_der_control_config();
std::shared_ptr<const IAcDerControlProvider> make_static_ac_der_control_provider(AcDerControlConfig config);

} // namespace iso15118::d20

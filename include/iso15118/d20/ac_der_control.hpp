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

enum class AcDerControlFailureReason {
    None,
    Unknown,
    NonAcDerServiceSelected,
    MissingSelectedControlFunctions,
    AcDerDisabled,
    StaleGridPolicy,
    StaleDsoControl,
    InvalidGridPolicy,
    InvalidDsoControl,
    InvalidEvseCapability,
    MissingSelectedMandatoryControlFunctions,
    MissingSupportedMandatoryControlFunctions,
    UnsupportedSelectedControlFunctions,
};

struct AcDerControlResult {
    std::optional<AcDerControlConfig> config;
    AcDerControlFailureReason failure_reason{AcDerControlFailureReason::None};
};

struct AcDerControlContext {
    dt::ServiceCategory selected_energy_service;
    dt::ControlMode selected_control_mode;
    dt::MobilityNeedsMode selected_mobility_needs_mode;
    std::optional<dt::DERControlFunctions> selected_der_control_functions;
};

struct AcDerGridPolicySnapshot {
    dt::RationalNumber volt_watt_start_voltage;
    dt::RationalNumber volt_watt_stop_voltage;
    dt::RationalNumber under_frequency_watt_start_hz;
    dt::RationalNumber under_frequency_watt_stop_hz;
    dt::RationalNumber over_frequency_watt_start_hz;
    dt::RationalNumber over_frequency_watt_stop_hz;
    dt::RationalNumber maximum_dc_injection;
    bool valid{false};
};

struct AcDerDsoControlSnapshot {
    dt::DSOQSetpoint q_setpoint;
    dt::DSOCosPhiSetpoint cos_phi_setpoint;
    bool valid{false};
};

struct AcDerEvseCapabilitySnapshot {
    dt::DERControlFunctions supported_control_functions;
    bool valid{false};
};

struct AcDerRuntimeStateSnapshot {
    bool ac_der_enabled{false};
    bool grid_policy_fresh{false};
    bool dso_control_fresh{false};
};

struct AcDerSeccControlSnapshots {
    AcDerGridPolicySnapshot grid_policy;
    AcDerDsoControlSnapshot dso_control;
    AcDerEvseCapabilitySnapshot evse_capability;
    AcDerRuntimeStateSnapshot runtime_state;
};

class IAcDerControlProvider {
public:
    virtual ~IAcDerControlProvider() = default;

    virtual std::optional<AcDerControlConfig> get_ac_der_control_config(const AcDerControlContext& context) const = 0;
    virtual AcDerControlResult get_ac_der_control_result(const AcDerControlContext& context) const;
};

AcDerControlConfig make_default_ac_der_control_config();
bool has_required_ac_der_control_functions(const dt::DERControlFunctions& controls);
bool validate_ac_der_grid_policy_snapshot(const AcDerGridPolicySnapshot& grid_policy);
bool validate_ac_der_dso_control_snapshot(const AcDerDsoControlSnapshot& dso_control);
bool validate_ac_der_control_config(const AcDerControlConfig& config, const dt::DERControlFunctions& controls);
const char* ac_der_control_failure_reason_to_string(AcDerControlFailureReason reason);
std::shared_ptr<const IAcDerControlProvider> make_static_ac_der_control_provider(AcDerControlConfig config);
AcDerSeccControlSnapshots make_default_ac_der_secc_control_snapshots();
std::shared_ptr<const IAcDerControlProvider> make_secc_ac_der_control_provider(AcDerSeccControlSnapshots snapshots);

} // namespace iso15118::d20

// SPDX-License-Identifier: Apache-2.0
// Vedecom 2026 : Contributors to EVerest
#include <iso15118/d20/ac_der_control.hpp>

#include <utility>

namespace iso15118::d20 {

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

class StaticAcDerControlProvider : public IAcDerControlProvider {
public:
    explicit StaticAcDerControlProvider(AcDerControlConfig config_) : config(std::move(config_)) {
    }

    std::optional<AcDerControlConfig> get_ac_der_control_config(const AcDerControlContext& context) const override {
        return get_ac_der_control_result(context).config;
    }

    AcDerControlResult get_ac_der_control_result(const AcDerControlContext& context) const override {
        if (context.selected_energy_service != dt::ServiceCategory::AC_DER) {
            return {std::nullopt, AcDerControlFailureReason::NonAcDerServiceSelected};
        }

        return {config, AcDerControlFailureReason::None};
    }

private:
    AcDerControlConfig config;
};

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

dt::FrequencyWatt make_frequency_watt(const dt::RationalNumber start_frequency,
                                      const dt::RationalNumber stop_frequency) {
    dt::FrequencyWatt frequency_watt;
    frequency_watt.f_start = start_frequency;
    frequency_watt.f_stop = stop_frequency;
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

dt::DERControlFunctions make_required_ac_der_control_functions() {
    dt::DERControlFunctions control_functions;

    control_functions.volt_watt = true;
    control_functions.dso_q_setpoint_provision = true;
    control_functions.dso_cos_phi_setpoint_provision = true;
    control_functions.dc_injection_restriction = true;
    control_functions.under_frequency_watt = true;
    control_functions.over_frequency_watt = true;
    control_functions.volt_var = true;
    control_functions.watt_var = true;
    control_functions.watt_cos_phi = true;
    control_functions.over_voltage_fault_ride_through = true;
    control_functions.under_voltage_fault_ride_through = true;
    control_functions.zero_current = true;

    control_functions.standard_bitmap = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) | (1u << 4) | (1u << 5);
    control_functions.extended_bitmap = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) | (1u << 4) | (1u << 5);

    return control_functions;
}

} // namespace

bool has_required_ac_der_control_functions(const dt::DERControlFunctions& controls) {
    return controls.volt_watt and controls.dso_q_setpoint_provision and controls.dso_cos_phi_setpoint_provision and
           controls.dc_injection_restriction and controls.under_frequency_watt and controls.over_frequency_watt and
           controls.volt_var and controls.watt_var and controls.watt_cos_phi and
           controls.over_voltage_fault_ride_through and controls.under_voltage_fault_ride_through and
           controls.zero_current;
}

const char* ac_der_control_failure_reason_to_string(const AcDerControlFailureReason reason) {
    switch (reason) {
    case AcDerControlFailureReason::None:
        return "none";
    case AcDerControlFailureReason::Unknown:
        return "unknown";
    case AcDerControlFailureReason::NonAcDerServiceSelected:
        return "non_ac_der_service_selected";
    case AcDerControlFailureReason::MissingSelectedControlFunctions:
        return "missing_selected_control_functions";
    case AcDerControlFailureReason::AcDerDisabled:
        return "ac_der_disabled";
    case AcDerControlFailureReason::StaleGridPolicy:
        return "stale_grid_policy";
    case AcDerControlFailureReason::StaleDsoControl:
        return "stale_dso_control";
    case AcDerControlFailureReason::InvalidGridPolicy:
        return "invalid_grid_policy";
    case AcDerControlFailureReason::InvalidDsoControl:
        return "invalid_dso_control";
    case AcDerControlFailureReason::InvalidEvseCapability:
        return "invalid_evse_capability";
    case AcDerControlFailureReason::MissingSelectedMandatoryControlFunctions:
        return "missing_selected_mandatory_control_functions";
    case AcDerControlFailureReason::MissingSupportedMandatoryControlFunctions:
        return "missing_supported_mandatory_control_functions";
    case AcDerControlFailureReason::UnsupportedSelectedControlFunctions:
        return "unsupported_selected_control_functions";
    }

    return "unknown";
}

namespace {

bool supports_selected_controls(const dt::DERControlFunctions& selected, const dt::DERControlFunctions& supported) {
    return (not selected.volt_watt or supported.volt_watt) and
           (not selected.dso_q_setpoint_provision or supported.dso_q_setpoint_provision) and
           (not selected.dso_cos_phi_setpoint_provision or supported.dso_cos_phi_setpoint_provision) and
           (not selected.dc_injection_restriction or supported.dc_injection_restriction) and
           (not selected.under_frequency_watt or supported.under_frequency_watt) and
           (not selected.over_frequency_watt or supported.over_frequency_watt) and
           (not selected.volt_var or supported.volt_var) and (not selected.watt_var or supported.watt_var) and
           (not selected.watt_cos_phi or supported.watt_cos_phi) and
           (not selected.over_voltage_fault_ride_through or supported.over_voltage_fault_ride_through) and
           (not selected.under_voltage_fault_ride_through or supported.under_voltage_fault_ride_through) and
           (not selected.zero_current or supported.zero_current);
}

bool snapshots_are_usable(const AcDerSeccControlSnapshots& snapshots) {
    return snapshots.runtime_state.ac_der_enabled and snapshots.runtime_state.grid_policy_fresh and
           snapshots.runtime_state.dso_control_fresh and snapshots.grid_policy.valid and snapshots.dso_control.valid and
           snapshots.evse_capability.valid;
}

AcDerControlFailureReason first_unusable_snapshot_reason(const AcDerSeccControlSnapshots& snapshots) {
    if (not snapshots.runtime_state.ac_der_enabled) {
        return AcDerControlFailureReason::AcDerDisabled;
    }
    if (not snapshots.runtime_state.grid_policy_fresh) {
        return AcDerControlFailureReason::StaleGridPolicy;
    }
    if (not snapshots.runtime_state.dso_control_fresh) {
        return AcDerControlFailureReason::StaleDsoControl;
    }
    if (not snapshots.grid_policy.valid) {
        return AcDerControlFailureReason::InvalidGridPolicy;
    }
    if (not snapshots.dso_control.valid) {
        return AcDerControlFailureReason::InvalidDsoControl;
    }
    if (not snapshots.evse_capability.valid) {
        return AcDerControlFailureReason::InvalidEvseCapability;
    }

    return AcDerControlFailureReason::None;
}

class SeccAcDerControlProvider : public IAcDerControlProvider {
public:
    explicit SeccAcDerControlProvider(AcDerSeccControlSnapshots snapshots_) : snapshots(std::move(snapshots_)) {
    }

    std::optional<AcDerControlConfig> get_ac_der_control_config(const AcDerControlContext& context) const override {
        return get_ac_der_control_result(context).config;
    }

    AcDerControlResult get_ac_der_control_result(const AcDerControlContext& context) const override {
        if (context.selected_energy_service != dt::ServiceCategory::AC_DER) {
            return {std::nullopt, AcDerControlFailureReason::NonAcDerServiceSelected};
        }
        if (not context.selected_der_control_functions.has_value()) {
            return {std::nullopt, AcDerControlFailureReason::MissingSelectedControlFunctions};
        }
        if (not snapshots_are_usable(snapshots)) {
            return {std::nullopt, first_unusable_snapshot_reason(snapshots)};
        }
        if (not has_required_ac_der_control_functions(*context.selected_der_control_functions)) {
            return {std::nullopt, AcDerControlFailureReason::MissingSelectedMandatoryControlFunctions};
        }
        if (not has_required_ac_der_control_functions(snapshots.evse_capability.supported_control_functions)) {
            return {std::nullopt, AcDerControlFailureReason::MissingSupportedMandatoryControlFunctions};
        }
        if (not supports_selected_controls(*context.selected_der_control_functions,
                                           snapshots.evse_capability.supported_control_functions)) {
            return {std::nullopt, AcDerControlFailureReason::UnsupportedSelectedControlFunctions};
        }

        auto config = make_default_ac_der_control_config();

        config.cpd_control.maximum_level_dc_injection = snapshots.grid_policy.maximum_dc_injection;
        config.dso_q_setpoint = snapshots.dso_control.q_setpoint;
        config.dso_cos_phi_setpoint = snapshots.dso_control.cos_phi_setpoint;

        auto& active_power_support = config.cpd_control.active_power_support.emplace();
        active_power_support.volt_watt = make_volt_watt();
        active_power_support.volt_watt->u_start = snapshots.grid_policy.volt_watt_start_voltage;
        active_power_support.volt_watt->u_stop = snapshots.grid_policy.volt_watt_stop_voltage;
        active_power_support.under_frequency_watt = make_frequency_watt(
            snapshots.grid_policy.under_frequency_watt_start_hz, snapshots.grid_policy.under_frequency_watt_stop_hz);
        active_power_support.over_frequency_watt = make_frequency_watt(
            snapshots.grid_policy.over_frequency_watt_start_hz, snapshots.grid_policy.over_frequency_watt_stop_hz);

        return {config, AcDerControlFailureReason::None};
    }

private:
    AcDerSeccControlSnapshots snapshots;
};

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

} // namespace

AcDerControlResult IAcDerControlProvider::get_ac_der_control_result(const AcDerControlContext& context) const {
    auto config = get_ac_der_control_config(context);
    if (config.has_value()) {
        return {config, AcDerControlFailureReason::None};
    }

    return {std::nullopt, AcDerControlFailureReason::Unknown};
}

AcDerControlConfig make_default_ac_der_control_config() {
    AcDerControlConfig config;

    config.cpd_control.overvoltage_fault_ride_through = make_fault_ride_through();
    config.cpd_control.undervoltage_fault_ride_through = make_fault_ride_through();
    config.cpd_control.zero_current = make_zero_current();

    auto& reactive_power_support = config.cpd_control.reactive_power_support.emplace();
    reactive_power_support.volt_var = make_der_curve(dt::DERCurveDataUnit::V, dt::DERCurveDataUnit::var);
    reactive_power_support.watt_var = make_der_curve(dt::DERCurveDataUnit::W, dt::DERCurveDataUnit::var);
    reactive_power_support.watt_cos_phi = make_der_curve(dt::DERCurveDataUnit::W, dt::DERCurveDataUnit::var);
    reactive_power_support.watt_cos_phi->curve_data_points[0].y_value.excitation =
        dt::DERPowerFactorExcitation::OverExcited;
    reactive_power_support.watt_cos_phi->curve_data_points[1].y_value.excitation =
        dt::DERPowerFactorExcitation::UnderExcited;

    auto& active_power_support = config.cpd_control.active_power_support.emplace();
    active_power_support.under_frequency_watt = make_frequency_watt(dt::from_float(49.8f), dt::from_float(49.5f));
    active_power_support.over_frequency_watt = make_frequency_watt(dt::from_float(50.2f), dt::from_float(50.5f));
    active_power_support.volt_watt = make_volt_watt();

    config.cpd_control.maximum_level_dc_injection = zero();
    config.dso_q_setpoint = {zero(), std::nullopt, std::nullopt, false, zero()};
    config.dso_cos_phi_setpoint = {one(), std::nullopt, std::nullopt, dt::DERPowerFactorExcitation::OverExcited,
                                   false, zero()};

    return config;
}

std::shared_ptr<const IAcDerControlProvider> make_static_ac_der_control_provider(AcDerControlConfig config) {
    return std::make_shared<StaticAcDerControlProvider>(std::move(config));
}

AcDerSeccControlSnapshots make_default_ac_der_secc_control_snapshots() {
    AcDerSeccControlSnapshots snapshots;

    snapshots.grid_policy.volt_watt_start_voltage = {241, 0};
    snapshots.grid_policy.volt_watt_stop_voltage = {253, 0};
    snapshots.grid_policy.under_frequency_watt_start_hz = {498, -1};
    snapshots.grid_policy.under_frequency_watt_stop_hz = {495, -1};
    snapshots.grid_policy.over_frequency_watt_start_hz = {502, -1};
    snapshots.grid_policy.over_frequency_watt_stop_hz = {505, -1};
    snapshots.grid_policy.maximum_dc_injection = {5, -1};
    snapshots.grid_policy.valid = true;

    snapshots.dso_control.q_setpoint = {{7, 2}, std::nullopt, std::nullopt, false, zero()};
    snapshots.dso_control.cos_phi_setpoint = {
        {95, -2}, std::nullopt, std::nullopt, dt::DERPowerFactorExcitation::UnderExcited, false, zero()};
    snapshots.dso_control.valid = true;

    snapshots.evse_capability.supported_control_functions = make_required_ac_der_control_functions();
    snapshots.evse_capability.valid = true;

    snapshots.runtime_state.ac_der_enabled = true;
    snapshots.runtime_state.grid_policy_fresh = true;
    snapshots.runtime_state.dso_control_fresh = true;

    return snapshots;
}

std::shared_ptr<const IAcDerControlProvider> make_secc_ac_der_control_provider(AcDerSeccControlSnapshots snapshots) {
    return std::make_shared<SeccAcDerControlProvider>(std::move(snapshots));
}

} // namespace iso15118::d20

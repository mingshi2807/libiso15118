// SPDX-License-Identifier: Apache-2.0
// Vedecom 2026 : Contributors to EVerest
#include <iso15118/d20/ac_der_control.hpp>

#include <utility>

namespace iso15118::d20 {

namespace {

float value_of(const dt::RationalNumber& value) {
    return dt::from_RationalNumber(value);
}

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
    case AcDerControlFailureReason::UnsupportedControlMode:
        return "unsupported_control_mode";
    case AcDerControlFailureReason::UnsupportedMobilityNeedsMode:
        return "unsupported_mobility_needs_mode";
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
        if (context.selected_control_mode != dt::ControlMode::Dynamic) {
            return {std::nullopt, AcDerControlFailureReason::UnsupportedControlMode};
        }
        if (context.selected_mobility_needs_mode != dt::MobilityNeedsMode::ProvidedByEvcc) {
            return {std::nullopt, AcDerControlFailureReason::UnsupportedMobilityNeedsMode};
        }
        if (not context.selected_der_control_functions.has_value()) {
            return {std::nullopt, AcDerControlFailureReason::MissingSelectedControlFunctions};
        }
        const auto snapshot_failure_reason = validate_ac_der_secc_control_snapshots(snapshots);
        if (snapshot_failure_reason != AcDerControlFailureReason::None) {
            return {std::nullopt, snapshot_failure_reason};
        }
        if (not supports_selected_controls(*context.selected_der_control_functions,
                                           snapshots.evse_capability.supported_control_functions)) {
            return {std::nullopt, AcDerControlFailureReason::UnsupportedSelectedControlFunctions};
        }
        if (not has_required_ac_der_control_functions(*context.selected_der_control_functions)) {
            return {std::nullopt, AcDerControlFailureReason::MissingSelectedMandatoryControlFunctions};
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

        if (not validate_ac_der_control_config(config, *context.selected_der_control_functions)) {
            return {std::nullopt, AcDerControlFailureReason::InvalidGridPolicy};
        }

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

namespace {

bool is_non_negative(const dt::RationalNumber& value) {
    return value_of(value) >= 0.0f;
}

bool is_positive(const dt::RationalNumber& value) {
    return value_of(value) > 0.0f;
}

bool is_power_factor(const dt::RationalNumber& value) {
    const auto cos_phi = value_of(value);
    return cos_phi >= 0.0f and cos_phi <= 1.0f;
}

bool optional_is_positive(const std::optional<dt::RationalNumber>& value) {
    return not value.has_value() or is_positive(*value);
}

bool optional_is_power_factor(const std::optional<dt::RationalNumber>& value) {
    return not value.has_value() or is_power_factor(*value);
}

bool optional_is_non_negative(const std::optional<dt::RationalNumber>& value) {
    return not value.has_value() or is_non_negative(*value);
}

bool validate_frequency_watt(const dt::FrequencyWatt& frequency_watt, const bool under_frequency_mode) {
    const auto f_start = value_of(frequency_watt.f_start);
    const auto f_stop = value_of(frequency_watt.f_stop);

    return is_positive(frequency_watt.f_start) and is_positive(frequency_watt.f_stop) and
           ((under_frequency_mode and f_start > f_stop) or (not under_frequency_mode and f_start < f_stop)) and
           is_non_negative(frequency_watt.slope) and
           is_non_negative(frequency_watt.step_response_time_constant_active_power);
}

bool validate_volt_watt(const dt::VoltWatt& volt_watt) {
    return is_positive(volt_watt.u_start) and is_positive(volt_watt.u_stop) and
           value_of(volt_watt.u_start) < value_of(volt_watt.u_stop) and
           is_non_negative(volt_watt.step_response_time_constant_active_power);
}

bool validate_der_curve(const dt::DERCurve& curve) {
    if (curve.curve_data_points.size() < 2 or not is_non_negative(curve.step_response_time_constant_reactive_power)) {
        return false;
    }

    auto previous_x = value_of(curve.curve_data_points.front().x_value);
    for (std::size_t index = 1; index < curve.curve_data_points.size(); index++) {
        const auto current_x = value_of(curve.curve_data_points[index].x_value);
        if (current_x <= previous_x) {
            return false;
        }
        previous_x = current_x;
    }

    return true;
}

bool validate_fault_ride_through(const dt::FaultRideThrough& fault_ride_through) {
    return is_positive(fault_ride_through.voltage_limit_start_frt) and
           optional_is_positive(fault_ride_through.voltage_limit_stop_frt) and
           optional_is_positive(fault_ride_through.voltage_recovery_limit) and
           is_non_negative(fault_ride_through.step_response_time_constant_active_power) and
           is_non_negative(fault_ride_through.step_response_time_constant_reactive_power);
}

bool validate_zero_current(const dt::ZeroCurrent& zero_current) {
    const auto has_voltage_limit = zero_current.over_voltage_limit.has_value() or zero_current.under_voltage_limit;

    return has_voltage_limit and optional_is_positive(zero_current.over_voltage_limit) and
           optional_is_positive(zero_current.under_voltage_limit) and
           optional_is_positive(zero_current.over_voltage_recovery_limit) and
           optional_is_positive(zero_current.under_voltage_recovery_limit) and
           is_non_negative(zero_current.step_response_time_constant_active_power) and
           is_non_negative(zero_current.step_response_time_constant_reactive_power);
}

bool validate_dso_q_setpoint(const dt::DSOQSetpoint& setpoint) {
    return is_non_negative(setpoint.step_response_time_constant_reactive_power);
}

bool validate_dso_cos_phi_setpoint(const dt::DSOCosPhiSetpoint& setpoint) {
    return is_power_factor(setpoint.value) and optional_is_power_factor(setpoint.value_L2) and
           optional_is_power_factor(setpoint.value_L3) and
           is_non_negative(setpoint.step_response_time_constant_reactive_power);
}

} // namespace

bool validate_ac_der_grid_policy_snapshot(const AcDerGridPolicySnapshot& grid_policy) {
    return value_of(grid_policy.volt_watt_start_voltage) < value_of(grid_policy.volt_watt_stop_voltage) and
           is_positive(grid_policy.volt_watt_start_voltage) and is_positive(grid_policy.volt_watt_stop_voltage) and
           value_of(grid_policy.under_frequency_watt_start_hz) > value_of(grid_policy.under_frequency_watt_stop_hz) and
           is_positive(grid_policy.under_frequency_watt_start_hz) and
           is_positive(grid_policy.under_frequency_watt_stop_hz) and
           value_of(grid_policy.over_frequency_watt_start_hz) < value_of(grid_policy.over_frequency_watt_stop_hz) and
           is_positive(grid_policy.over_frequency_watt_start_hz) and
           is_positive(grid_policy.over_frequency_watt_stop_hz) and is_non_negative(grid_policy.maximum_dc_injection);
}

bool validate_ac_der_dso_control_snapshot(const AcDerDsoControlSnapshot& dso_control) {
    return validate_dso_q_setpoint(dso_control.q_setpoint) and
           validate_dso_cos_phi_setpoint(dso_control.cos_phi_setpoint);
}

bool validate_ac_der_control_config(const AcDerControlConfig& config, const dt::DERControlFunctions& controls) {
    const auto& der_control = config.cpd_control;
    const auto has_active_power = der_control.active_power_support.has_value();
    const auto has_reactive_power = der_control.reactive_power_support.has_value();

    return (not controls.volt_watt or (has_active_power and der_control.active_power_support->volt_watt.has_value() and
                                       validate_volt_watt(*der_control.active_power_support->volt_watt))) and
           (not controls.under_frequency_watt or
            (has_active_power and der_control.active_power_support->under_frequency_watt.has_value() and
             validate_frequency_watt(*der_control.active_power_support->under_frequency_watt, true))) and
           (not controls.over_frequency_watt or
            (has_active_power and der_control.active_power_support->over_frequency_watt.has_value() and
             validate_frequency_watt(*der_control.active_power_support->over_frequency_watt, false))) and
           (not controls.volt_var or
            (has_reactive_power and der_control.reactive_power_support->volt_var.has_value() and
             validate_der_curve(*der_control.reactive_power_support->volt_var))) and
           (not controls.watt_var or
            (has_reactive_power and der_control.reactive_power_support->watt_var.has_value() and
             validate_der_curve(*der_control.reactive_power_support->watt_var))) and
           (not controls.watt_cos_phi or
            (has_reactive_power and der_control.reactive_power_support->watt_cos_phi.has_value() and
             validate_der_curve(*der_control.reactive_power_support->watt_cos_phi))) and
           (not controls.over_voltage_fault_ride_through or
            (der_control.overvoltage_fault_ride_through.has_value() and
             validate_fault_ride_through(*der_control.overvoltage_fault_ride_through))) and
           (not controls.under_voltage_fault_ride_through or
            (der_control.undervoltage_fault_ride_through.has_value() and
             validate_fault_ride_through(*der_control.undervoltage_fault_ride_through))) and
           (not controls.zero_current or
            (der_control.zero_current.has_value() and validate_zero_current(*der_control.zero_current))) and
           (not controls.dc_injection_restriction or (der_control.maximum_level_dc_injection.has_value() and
                                                      is_non_negative(*der_control.maximum_level_dc_injection))) and
           (not controls.dso_q_setpoint_provision or validate_dso_q_setpoint(config.dso_q_setpoint)) and
           (not controls.dso_cos_phi_setpoint_provision or validate_dso_cos_phi_setpoint(config.dso_cos_phi_setpoint));
}

AcDerControlFailureReason validate_ac_der_secc_control_snapshots(const AcDerSeccControlSnapshots& snapshots) {
    if (not snapshots.runtime_state.ac_der_enabled) {
        return AcDerControlFailureReason::AcDerDisabled;
    }
    if (not snapshots.runtime_state.grid_policy_fresh) {
        return AcDerControlFailureReason::StaleGridPolicy;
    }
    if (not snapshots.runtime_state.dso_control_fresh) {
        return AcDerControlFailureReason::StaleDsoControl;
    }
    if (not snapshots.grid_policy.valid or not validate_ac_der_grid_policy_snapshot(snapshots.grid_policy)) {
        return AcDerControlFailureReason::InvalidGridPolicy;
    }
    if (not snapshots.dso_control.valid or not validate_ac_der_dso_control_snapshot(snapshots.dso_control)) {
        return AcDerControlFailureReason::InvalidDsoControl;
    }
    if (not snapshots.evse_capability.valid) {
        return AcDerControlFailureReason::InvalidEvseCapability;
    }
    if (not has_required_ac_der_control_functions(snapshots.evse_capability.supported_control_functions)) {
        return AcDerControlFailureReason::MissingSupportedMandatoryControlFunctions;
    }

    return AcDerControlFailureReason::None;
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

    snapshots.grid_policy.volt_watt_start_voltage = {230, 0};
    snapshots.grid_policy.volt_watt_stop_voltage = {253, 0};
    snapshots.grid_policy.under_frequency_watt_start_hz = {498, -1};
    snapshots.grid_policy.under_frequency_watt_stop_hz = {495, -1};
    snapshots.grid_policy.over_frequency_watt_start_hz = {502, -1};
    snapshots.grid_policy.over_frequency_watt_stop_hz = {505, -1};
    snapshots.grid_policy.maximum_dc_injection = zero();
    snapshots.grid_policy.valid = true;

    snapshots.dso_control.q_setpoint = {zero(), std::nullopt, std::nullopt, false, zero()};
    snapshots.dso_control.cos_phi_setpoint = {
        one(), std::nullopt, std::nullopt, dt::DERPowerFactorExcitation::OverExcited, false, zero()};
    snapshots.dso_control.valid = true;

    snapshots.evse_capability.supported_control_functions = make_required_ac_der_control_functions();
    snapshots.evse_capability.valid = true;

    snapshots.runtime_state.ac_der_enabled = true;
    snapshots.runtime_state.grid_policy_fresh = true;
    snapshots.runtime_state.dso_control_fresh = true;

    return snapshots;
}

AcDerSeccControlSnapshots make_ac_der_iec_dynamic_eim_profile_snapshots() {
    auto snapshots = make_default_ac_der_secc_control_snapshots();

    snapshots.grid_policy.volt_watt_start_voltage = {241, 0};
    snapshots.grid_policy.volt_watt_stop_voltage = {253, 0};
    snapshots.grid_policy.maximum_dc_injection = {5, -1};

    snapshots.dso_control.q_setpoint = {{7, 2}, std::nullopt, std::nullopt, false, zero()};
    snapshots.dso_control.cos_phi_setpoint = {
        {95, -2}, std::nullopt, std::nullopt, dt::DERPowerFactorExcitation::UnderExcited, false, zero()};

    return snapshots;
}

std::shared_ptr<const IAcDerControlProvider> make_secc_ac_der_control_provider(AcDerSeccControlSnapshots snapshots) {
    return std::make_shared<SeccAcDerControlProvider>(std::move(snapshots));
}

} // namespace iso15118::d20

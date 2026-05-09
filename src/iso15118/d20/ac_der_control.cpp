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
        if (context.selected_energy_service != dt::ServiceCategory::AC_DER) {
            return std::nullopt;
        }

        return config;
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

} // namespace

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
    active_power_support.under_frequency_watt = make_frequency_watt(49.8f, 49.5f);
    active_power_support.over_frequency_watt = make_frequency_watt(50.2f, 50.5f);
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

} // namespace iso15118::d20

// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Vedecom Contributors to EVerest
#include <iso15118/d20/ac_der_control.hpp>
#include <iso15118/d20/config.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <utility>

namespace d20 = iso15118::d20;
namespace dt = iso15118::message_20::datatypes;

namespace {

const char* to_string(const dt::ServiceCategory service) {
    switch (service) {
    case dt::ServiceCategory::AC_DER:
        return "AC_DER";
    case dt::ServiceCategory::AC_BPT:
        return "AC_BPT";
    case dt::ServiceCategory::AC:
        return "AC";
    default:
        return "other";
    }
}

const char* to_string(const dt::ControlMode mode) {
    switch (mode) {
    case dt::ControlMode::Dynamic:
        return "Dynamic";
    case dt::ControlMode::Scheduled:
        return "Scheduled";
    }
    return "unknown";
}

const char* to_string(const dt::MobilityNeedsMode mode) {
    switch (mode) {
    case dt::MobilityNeedsMode::ProvidedByEvcc:
        return "ProvidedByEvcc";
    case dt::MobilityNeedsMode::ProvidedBySecc:
        return "ProvidedBySecc";
    }
    return "unknown";
}

const char* to_string(const dt::DERPowerFactorExcitation excitation) {
    switch (excitation) {
    case dt::DERPowerFactorExcitation::OverExcited:
        return "OverExcited";
    case dt::DERPowerFactorExcitation::UnderExcited:
        return "UnderExcited";
    }
    return "unknown";
}

float value_of(const dt::RationalNumber& value) {
    return dt::from_RationalNumber(value);
}

void print_snapshot_status(const d20::AcDerSeccControlSnapshots& snapshots) {
    std::cout << "provider snapshot status:\n";
    std::cout << "  AC DER enabled: " << (snapshots.runtime_state.ac_der_enabled ? "yes" : "no") << "\n";
    std::cout << "  grid policy fresh: " << (snapshots.runtime_state.grid_policy_fresh ? "yes" : "no") << "\n";
    std::cout << "  DSO control fresh: " << (snapshots.runtime_state.dso_control_fresh ? "yes" : "no") << "\n";
    std::cout << "  EVSE capabilities valid: " << (snapshots.evse_capability.valid ? "yes" : "no") << "\n";
}

void print_selected_control_functions(const dt::DERControlFunctions& functions) {
    std::cout << "selected DER control functions:\n";
    std::cout << "  VoltWatt: " << (functions.volt_watt ? "yes" : "no") << "\n";
    std::cout << "  DSOQSetPointProvision: " << (functions.dso_q_setpoint_provision ? "yes" : "no") << "\n";
    std::cout << "  DSOQCosphiSetPointProvision: " << (functions.dso_cos_phi_setpoint_provision ? "yes" : "no") << "\n";
    std::cout << "  DCInjectionRestriction: " << (functions.dc_injection_restriction ? "yes" : "no") << "\n";
    std::cout << "  UnderFrequencyWatt: " << (functions.under_frequency_watt ? "yes" : "no") << "\n";
    std::cout << "  OverFrequencyWatt: " << (functions.over_frequency_watt ? "yes" : "no") << "\n";
    std::cout << "  VoltVar: " << (functions.volt_var ? "yes" : "no") << "\n";
    std::cout << "  WattVar: " << (functions.watt_var ? "yes" : "no") << "\n";
    std::cout << "  WattCosPhi: " << (functions.watt_cos_phi ? "yes" : "no") << "\n";
    std::cout << "  OverVoltageFRT: " << (functions.over_voltage_fault_ride_through ? "yes" : "no") << "\n";
    std::cout << "  UnderVoltageFRT: " << (functions.under_voltage_fault_ride_through ? "yes" : "no") << "\n";
    std::cout << "  ZeroCurrent: " << (functions.zero_current ? "yes" : "no") << "\n";
}

void print_ac_der_config(const d20::AcDerControlContext& context, const d20::AcDerControlConfig& config,
                         const d20::AcDerSeccControlSnapshots& snapshots) {
    std::cout << "AC DER SECC provider demo\n";
    print_snapshot_status(snapshots);
    std::cout << "selected service: " << to_string(context.selected_energy_service) << "\n";
    std::cout << "selected control mode: " << to_string(context.selected_control_mode) << "\n";
    std::cout << "selected mobility needs mode: " << to_string(context.selected_mobility_needs_mode) << "\n";
    std::cout << "DER control functions selected: "
              << (context.selected_der_control_functions.has_value() ? "yes" : "no") << "\n";
    if (context.selected_der_control_functions.has_value()) {
        print_selected_control_functions(*context.selected_der_control_functions);
    }

    if (config.cpd_control.maximum_level_dc_injection.has_value()) {
        std::cout << "maximum DC injection: " << value_of(*config.cpd_control.maximum_level_dc_injection) << " A\n";
    }

    if (config.cpd_control.active_power_support.has_value()) {
        const auto& active_power = *config.cpd_control.active_power_support;
        std::cout << "active power support:\n";
        std::cout << "  VoltWatt: " << (active_power.volt_watt.has_value() ? "configured" : "missing") << "\n";
        if (active_power.volt_watt.has_value()) {
            std::cout << "  VoltWatt Ustart: " << value_of(active_power.volt_watt->u_start) << " V\n";
            std::cout << "  VoltWatt Ustop: " << value_of(active_power.volt_watt->u_stop) << " V\n";
        }
        std::cout << "  UnderFrequencyWatt: "
                  << (active_power.under_frequency_watt.has_value() ? "configured" : "missing") << "\n";
        if (active_power.under_frequency_watt.has_value()) {
            std::cout << "  UnderFrequencyWatt Fstart: " << value_of(active_power.under_frequency_watt->f_start)
                      << " Hz\n";
            std::cout << "  UnderFrequencyWatt Fstop: " << value_of(active_power.under_frequency_watt->f_stop)
                      << " Hz\n";
        }
        std::cout << "  OverFrequencyWatt: "
                  << (active_power.over_frequency_watt.has_value() ? "configured" : "missing") << "\n";
        if (active_power.over_frequency_watt.has_value()) {
            std::cout << "  OverFrequencyWatt Fstart: " << value_of(active_power.over_frequency_watt->f_start)
                      << " Hz\n";
            std::cout << "  OverFrequencyWatt Fstop: " << value_of(active_power.over_frequency_watt->f_stop) << " Hz\n";
        }
    }

    if (config.cpd_control.reactive_power_support.has_value()) {
        const auto& reactive_power = *config.cpd_control.reactive_power_support;
        std::cout << "reactive power support:\n";
        std::cout << "  VoltVar: " << (reactive_power.volt_var.has_value() ? "configured" : "missing") << "\n";
        std::cout << "  WattVar: " << (reactive_power.watt_var.has_value() ? "configured" : "missing") << "\n";
        std::cout << "  WattCosPhi: " << (reactive_power.watt_cos_phi.has_value() ? "configured" : "missing") << "\n";
    }

    std::cout << "DSOQ setpoint: " << value_of(config.dso_q_setpoint.value) << " var\n";
    std::cout << "DSOCosPhi setpoint: " << value_of(config.dso_cos_phi_setpoint.value) << " ("
              << to_string(config.dso_cos_phi_setpoint.excitation) << ")\n";
    std::cout << "overvoltage FRT: "
              << (config.cpd_control.overvoltage_fault_ride_through.has_value() ? "configured" : "missing") << "\n";
    std::cout << "undervoltage FRT: "
              << (config.cpd_control.undervoltage_fault_ride_through.has_value() ? "configured" : "missing") << "\n";
    std::cout << "zero current mode: " << (config.cpd_control.zero_current.has_value() ? "configured" : "missing")
              << "\n";
}

d20::DcTransferLimits make_dc_limits() {
    const auto zero = dt::RationalNumber{0, 0};
    return {{{zero, zero}, {zero, zero}}, std::nullopt, {zero, zero}, std::nullopt};
}

d20::AcTransferLimits make_ac_limits() {
    return {{dt::from_float(22000.0f), dt::from_float(1000.0f)},
            std::nullopt,
            std::nullopt,
            dt::from_float(50.0f),
            std::nullopt,
            std::nullopt,
            d20::Limit<dt::RationalNumber>{dt::from_float(11000.0f), dt::from_float(0.0f)},
            std::nullopt,
            std::nullopt};
}

d20::EvseSetupConfig make_evse_setup_config(std::shared_ptr<const d20::IAcDerControlProvider> provider) {
    return {"SECC-DER-DEMO",
            {dt::ServiceCategory::AC_DER},
            {dt::Authorization::EIM},
            {},
            false,
            make_dc_limits(),
            make_ac_limits(),
            {{dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc}},
            std::nullopt,
            d20::AcSetupConfig{230, {dt::AcConnector::ThreePhase}},
            d20::BptSetupConfig{dt::BptChannel::Unified, dt::GeneratorMode::GridFollowing,
                                dt::GridCodeIslandingDetectionMethod::Passive},
            make_dc_limits(),
            std::nullopt,
            std::move(provider)};
}

} // namespace

int main() {
    auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
    snapshots.grid_policy.volt_watt_start_voltage = {241, 0};
    snapshots.dso_control.q_setpoint.value = {7, 2};
    snapshots.dso_control.cos_phi_setpoint.value = {95, -2};

    auto provider = d20::make_secc_ac_der_control_provider(snapshots);
    const auto setup = make_evse_setup_config(provider);
    const auto session_config = d20::SessionConfig(setup);

    const auto ac_der_context = d20::AcDerControlContext{dt::ServiceCategory::AC_DER, dt::ControlMode::Dynamic,
                                                         dt::MobilityNeedsMode::ProvidedByEvcc,
                                                         snapshots.evse_capability.supported_control_functions};
    const auto ac_der_config = session_config.ac_der_control_provider->get_ac_der_control_config(ac_der_context);

    if (not ac_der_config.has_value()) {
        std::cerr << "AC DER provider could not produce a configuration\n";
        return 1;
    }

    print_ac_der_config(ac_der_context, *ac_der_config, snapshots);
    return 0;
}

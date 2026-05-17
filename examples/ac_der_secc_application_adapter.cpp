// SPDX-License-Identifier: Apache-2.0
// 2026 Vedecom Contributors to EVerest
#include <iso15118/d20/ac_der_control.hpp>
#include <iso15118/d20/config.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace d20 = iso15118::d20;
namespace dt = iso15118::message_20::datatypes;

namespace {

struct GridCodePolicy {
    float volt_watt_start_voltage{241.0f};
    float volt_watt_stop_voltage{253.0f};
    float under_frequency_watt_start_hz{49.8f};
    float under_frequency_watt_stop_hz{47.5f};
    float over_frequency_watt_start_hz{50.2f};
    float over_frequency_watt_stop_hz{51.5f};
    float maximum_dc_injection{0.5f};
    bool valid{true};
};

struct DsoControlCommand {
    dt::RationalNumber q_setpoint_var{7, 2};
    dt::RationalNumber cos_phi{95, -2};
    dt::DERPowerFactorExcitation cos_phi_excitation{dt::DERPowerFactorExcitation::UnderExcited};
    bool valid{true};
};

struct EvseDerCapability {
    dt::DERControlFunctions supported_functions;
    bool valid{true};
};

struct RuntimeHealth {
    bool ac_der_enabled{true};
    bool grid_policy_fresh{true};
    bool dso_control_fresh{true};
};

float value_of(const dt::RationalNumber& value) {
    return dt::from_RationalNumber(value);
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

dt::DERControlFunctions make_required_control_functions() {
    auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
    return snapshots.evse_capability.supported_control_functions;
}

d20::AcDerSeccControlSnapshots make_protocol_snapshots(const GridCodePolicy& grid_policy,
                                                       const DsoControlCommand& dso_command,
                                                       const EvseDerCapability& evse_capability,
                                                       const RuntimeHealth& runtime_health) {
    auto snapshots = d20::make_default_ac_der_secc_control_snapshots();

    snapshots.grid_policy.volt_watt_start_voltage = dt::from_float(grid_policy.volt_watt_start_voltage);
    snapshots.grid_policy.volt_watt_stop_voltage = dt::from_float(grid_policy.volt_watt_stop_voltage);
    snapshots.grid_policy.under_frequency_watt_start_hz = dt::from_float(grid_policy.under_frequency_watt_start_hz);
    snapshots.grid_policy.under_frequency_watt_stop_hz = dt::from_float(grid_policy.under_frequency_watt_stop_hz);
    snapshots.grid_policy.over_frequency_watt_start_hz = dt::from_float(grid_policy.over_frequency_watt_start_hz);
    snapshots.grid_policy.over_frequency_watt_stop_hz = dt::from_float(grid_policy.over_frequency_watt_stop_hz);
    snapshots.grid_policy.maximum_dc_injection = dt::from_float(grid_policy.maximum_dc_injection);
    snapshots.grid_policy.valid = grid_policy.valid;

    snapshots.dso_control.q_setpoint.value = dso_command.q_setpoint_var;
    snapshots.dso_control.cos_phi_setpoint.value = dso_command.cos_phi;
    snapshots.dso_control.cos_phi_setpoint.excitation = dso_command.cos_phi_excitation;
    snapshots.dso_control.valid = dso_command.valid;

    snapshots.evse_capability.supported_control_functions = evse_capability.supported_functions;
    snapshots.evse_capability.valid = evse_capability.valid;

    snapshots.runtime_state.ac_der_enabled = runtime_health.ac_der_enabled;
    snapshots.runtime_state.grid_policy_fresh = runtime_health.grid_policy_fresh;
    snapshots.runtime_state.dso_control_fresh = runtime_health.dso_control_fresh;

    return snapshots;
}

class SeccApplicationAcDerAdapter {
public:
    SeccApplicationAcDerAdapter(GridCodePolicy grid_policy_, DsoControlCommand dso_command_,
                                EvseDerCapability evse_capability_, RuntimeHealth runtime_health_) :
        snapshots(make_protocol_snapshots(grid_policy_, dso_command_, evse_capability_, runtime_health_)),
        provider(d20::make_secc_ac_der_control_provider(snapshots)) {
    }

    d20::SessionConfig make_session_config() const {
        d20::EvseSetupConfig setup{"SECC-PRODUCTION-AC-DER-IEC",
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
                                   provider};

        return d20::SessionConfig(setup);
    }

    const d20::AcDerSeccControlSnapshots& get_snapshots() const {
        return snapshots;
    }

private:
    d20::AcDerSeccControlSnapshots snapshots;
    std::shared_ptr<const d20::IAcDerControlProvider> provider;
};

void print_runtime_gates(const d20::AcDerSeccControlSnapshots& snapshots) {
    std::cout << "runtime gates\n";
    std::cout << "  ac_der_enabled=" << (snapshots.runtime_state.ac_der_enabled ? "true" : "false") << "\n";
    std::cout << "  grid_policy_fresh=" << (snapshots.runtime_state.grid_policy_fresh ? "true" : "false") << "\n";
    std::cout << "  dso_control_fresh=" << (snapshots.runtime_state.dso_control_fresh ? "true" : "false") << "\n";
    std::cout << "  grid_policy_valid=" << (snapshots.grid_policy.valid ? "true" : "false") << "\n";
    std::cout << "  dso_control_valid=" << (snapshots.dso_control.valid ? "true" : "false") << "\n";
    std::cout << "  evse_capability_valid=" << (snapshots.evse_capability.valid ? "true" : "false") << "\n";
}

void print_config_summary(const d20::AcDerControlConfig& config) {
    std::cout << "accepted AC_DER_IEC control contract\n";
    if (config.cpd_control.active_power_support.has_value() and
        config.cpd_control.active_power_support->volt_watt.has_value()) {
        const auto& volt_watt = *config.cpd_control.active_power_support->volt_watt;
        std::cout << "  VoltWatt.Ustart=" << value_of(volt_watt.u_start) << " V\n";
        std::cout << "  VoltWatt.Ustop=" << value_of(volt_watt.u_stop) << " V\n";
    }
    if (config.cpd_control.maximum_level_dc_injection.has_value()) {
        std::cout << "  DCInjectionRestriction=" << value_of(*config.cpd_control.maximum_level_dc_injection) << " A\n";
    }
    std::cout << "  DSOQSetPointProvision=" << value_of(config.dso_q_setpoint.value) << " var\n";
    std::cout << "  DSOQCosphiSetPointProvision=" << value_of(config.dso_cos_phi_setpoint.value) << "\n";
}

bool request_provider_config(const d20::SessionConfig& session_config, const d20::AcDerControlContext& context,
                             const std::string& label) {
    const auto config = session_config.ac_der_control_provider->get_ac_der_control_config(context);
    std::cout << label << ": " << (config.has_value() ? "accepted" : "rejected") << "\n";
    if (config.has_value()) {
        print_config_summary(*config);
    }
    return config.has_value();
}

} // namespace

int main() {
    const auto required_functions = make_required_control_functions();

    const SeccApplicationAcDerAdapter adapter(GridCodePolicy{}, DsoControlCommand{},
                                              EvseDerCapability{required_functions, true}, RuntimeHealth{});
    const auto session_config = adapter.make_session_config();

    print_runtime_gates(adapter.get_snapshots());

    const auto ac_der_context = d20::AcDerControlContext{dt::ServiceCategory::AC_DER, dt::ControlMode::Dynamic,
                                                         dt::MobilityNeedsMode::ProvidedByEvcc, required_functions};
    const auto ac_bpt_context = d20::AcDerControlContext{dt::ServiceCategory::AC_BPT, dt::ControlMode::Dynamic,
                                                         dt::MobilityNeedsMode::ProvidedByEvcc, required_functions};

    const auto accepted = request_provider_config(session_config, ac_der_context, "AC_DER selected service");
    const auto rejected = not request_provider_config(session_config, ac_bpt_context, "AC_BPT selected service");

    if (not accepted or not rejected) {
        std::cerr << "SECC AC DER adapter demo failed\n";
        return 1;
    }

    return 0;
}

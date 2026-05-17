// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Vedecom Contributors to EVerest
#include <catch2/catch_test_macros.hpp>

#include <iso15118/d20/ac_der_control.hpp>

namespace d20 = iso15118::d20;
namespace dt = iso15118::message_20::datatypes;

namespace {

struct GridCodePolicy {
    dt::RationalNumber volt_watt_start_voltage{242, 0};
    dt::RationalNumber volt_watt_stop_voltage{254, 0};
    dt::RationalNumber under_frequency_watt_start_hz{497, -1};
    dt::RationalNumber under_frequency_watt_stop_hz{492, -1};
    dt::RationalNumber over_frequency_watt_start_hz{503, -1};
    dt::RationalNumber over_frequency_watt_stop_hz{508, -1};
    dt::RationalNumber maximum_dc_injection{3, -1};
    bool valid{true};
};

struct DsoControlCommand {
    dt::DSOQSetpoint q_setpoint{{9, 2}, std::nullopt, std::nullopt, false, {0, 0}};
    dt::DSOCosPhiSetpoint cos_phi_setpoint{
        {97, -2}, std::nullopt, std::nullopt, dt::DERPowerFactorExcitation::UnderExcited, false, {0, 0}};
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

dt::DERControlFunctions required_control_functions() {
    return d20::make_default_ac_der_secc_control_snapshots().evse_capability.supported_control_functions;
}

d20::AcDerSeccControlSnapshots make_snapshots(const GridCodePolicy& grid_policy, const DsoControlCommand& dso_command,
                                              const EvseDerCapability& evse_capability,
                                              const RuntimeHealth& runtime_health) {
    d20::AcDerSeccControlSnapshots snapshots;

    snapshots.grid_policy.volt_watt_start_voltage = grid_policy.volt_watt_start_voltage;
    snapshots.grid_policy.volt_watt_stop_voltage = grid_policy.volt_watt_stop_voltage;
    snapshots.grid_policy.under_frequency_watt_start_hz = grid_policy.under_frequency_watt_start_hz;
    snapshots.grid_policy.under_frequency_watt_stop_hz = grid_policy.under_frequency_watt_stop_hz;
    snapshots.grid_policy.over_frequency_watt_start_hz = grid_policy.over_frequency_watt_start_hz;
    snapshots.grid_policy.over_frequency_watt_stop_hz = grid_policy.over_frequency_watt_stop_hz;
    snapshots.grid_policy.maximum_dc_injection = grid_policy.maximum_dc_injection;
    snapshots.grid_policy.valid = grid_policy.valid;

    snapshots.dso_control.q_setpoint = dso_command.q_setpoint;
    snapshots.dso_control.cos_phi_setpoint = dso_command.cos_phi_setpoint;
    snapshots.dso_control.valid = dso_command.valid;

    snapshots.evse_capability.supported_control_functions = evse_capability.supported_functions;
    snapshots.evse_capability.valid = evse_capability.valid;

    snapshots.runtime_state.ac_der_enabled = runtime_health.ac_der_enabled;
    snapshots.runtime_state.grid_policy_fresh = runtime_health.grid_policy_fresh;
    snapshots.runtime_state.dso_control_fresh = runtime_health.dso_control_fresh;

    return snapshots;
}

class SeccApplicationAdapter {
public:
    SeccApplicationAdapter(GridCodePolicy grid_policy, DsoControlCommand dso_command, EvseDerCapability evse_capability,
                           RuntimeHealth runtime_health) :
        snapshots(make_snapshots(grid_policy, dso_command, evse_capability, runtime_health)),
        provider(d20::make_secc_ac_der_control_provider(snapshots)) {
    }

    std::optional<d20::AcDerControlConfig> request_config(dt::ServiceCategory service,
                                                          dt::DERControlFunctions selected_functions) const {
        return provider->get_ac_der_control_config(
            {service, dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc, selected_functions});
    }

private:
    d20::AcDerSeccControlSnapshots snapshots;
    std::shared_ptr<const d20::IAcDerControlProvider> provider;
};

void require_mandatory_control_payloads(const d20::AcDerControlConfig& config) {
    REQUIRE(config.cpd_control.active_power_support.has_value());
    REQUIRE(config.cpd_control.active_power_support->volt_watt.has_value());
    REQUIRE(config.cpd_control.active_power_support->under_frequency_watt.has_value());
    REQUIRE(config.cpd_control.active_power_support->over_frequency_watt.has_value());
    REQUIRE(config.cpd_control.reactive_power_support.has_value());
    REQUIRE(config.cpd_control.reactive_power_support->volt_var.has_value());
    REQUIRE(config.cpd_control.reactive_power_support->watt_var.has_value());
    REQUIRE(config.cpd_control.reactive_power_support->watt_cos_phi.has_value());
    REQUIRE(config.cpd_control.overvoltage_fault_ride_through.has_value());
    REQUIRE(config.cpd_control.zero_current.has_value());
    REQUIRE(config.cpd_control.maximum_level_dc_injection.has_value());
}

} // namespace

SCENARIO("AC DER SECC application adapter boundary") {
    const auto required_functions = required_control_functions();

    GIVEN("fresh production application inputs and an AC_DER selected service") {
        const SeccApplicationAdapter adapter(GridCodePolicy{}, DsoControlCommand{},
                                             EvseDerCapability{required_functions, true}, RuntimeHealth{});

        const auto config = adapter.request_config(dt::ServiceCategory::AC_DER, required_functions);

        REQUIRE(config.has_value());
        require_mandatory_control_payloads(*config);
        REQUIRE(value_of(config->cpd_control.active_power_support->volt_watt->u_start) == 242.0f);
        REQUIRE(value_of(config->cpd_control.active_power_support->volt_watt->u_stop) == 254.0f);
        REQUIRE(value_of(config->cpd_control.active_power_support->under_frequency_watt->f_start) == 49.7f);
        REQUIRE(value_of(config->cpd_control.active_power_support->under_frequency_watt->f_stop) == 49.2f);
        REQUIRE(value_of(config->cpd_control.active_power_support->over_frequency_watt->f_start) == 50.3f);
        REQUIRE(value_of(config->cpd_control.active_power_support->over_frequency_watt->f_stop) == 50.8f);
        REQUIRE(value_of(*config->cpd_control.maximum_level_dc_injection) == 0.3f);
        REQUIRE(value_of(config->dso_q_setpoint.value) == 900.0f);
        REQUIRE(value_of(config->dso_cos_phi_setpoint.value) == 0.97f);
        REQUIRE(config->dso_cos_phi_setpoint.excitation == dt::DERPowerFactorExcitation::UnderExcited);
    }

    GIVEN("fresh production application inputs but AC_BPT is selected") {
        const SeccApplicationAdapter adapter(GridCodePolicy{}, DsoControlCommand{},
                                             EvseDerCapability{required_functions, true}, RuntimeHealth{});

        const auto config = adapter.request_config(dt::ServiceCategory::AC_BPT, required_functions);

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("an incomplete mandatory AC_DER IEC capability bitmap") {
        auto incomplete_functions = required_functions;
        incomplete_functions.volt_watt = false;
        const SeccApplicationAdapter adapter(GridCodePolicy{}, DsoControlCommand{},
                                             EvseDerCapability{incomplete_functions, true}, RuntimeHealth{});

        const auto config = adapter.request_config(dt::ServiceCategory::AC_DER, incomplete_functions);

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("fresh EVSE capability but an incomplete EVCC-selected mandatory bitmap") {
        auto incomplete_selected_functions = required_functions;
        incomplete_selected_functions.zero_current = false;
        const SeccApplicationAdapter adapter(GridCodePolicy{}, DsoControlCommand{},
                                             EvseDerCapability{required_functions, true}, RuntimeHealth{});

        const auto config = adapter.request_config(dt::ServiceCategory::AC_DER, incomplete_selected_functions);

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("stale or invalid application inputs") {
        SECTION("stale grid policy") {
            const SeccApplicationAdapter adapter(GridCodePolicy{}, DsoControlCommand{},
                                                 EvseDerCapability{required_functions, true},
                                                 RuntimeHealth{true, false, true});

            REQUIRE_FALSE(adapter.request_config(dt::ServiceCategory::AC_DER, required_functions).has_value());
        }

        SECTION("stale DSO command") {
            const SeccApplicationAdapter adapter(GridCodePolicy{}, DsoControlCommand{},
                                                 EvseDerCapability{required_functions, true},
                                                 RuntimeHealth{true, true, false});

            REQUIRE_FALSE(adapter.request_config(dt::ServiceCategory::AC_DER, required_functions).has_value());
        }

        SECTION("invalid EVSE capability") {
            const SeccApplicationAdapter adapter(GridCodePolicy{}, DsoControlCommand{},
                                                 EvseDerCapability{required_functions, false}, RuntimeHealth{});

            REQUIRE_FALSE(adapter.request_config(dt::ServiceCategory::AC_DER, required_functions).has_value());
        }

        SECTION("invalid grid policy") {
            auto invalid_grid_policy = GridCodePolicy{};
            invalid_grid_policy.valid = false;
            const SeccApplicationAdapter adapter(invalid_grid_policy, DsoControlCommand{},
                                                 EvseDerCapability{required_functions, true}, RuntimeHealth{});

            REQUIRE_FALSE(adapter.request_config(dt::ServiceCategory::AC_DER, required_functions).has_value());
        }

        SECTION("invalid DSO command") {
            auto invalid_dso_command = DsoControlCommand{};
            invalid_dso_command.valid = false;
            const SeccApplicationAdapter adapter(GridCodePolicy{}, invalid_dso_command,
                                                 EvseDerCapability{required_functions, true}, RuntimeHealth{});

            REQUIRE_FALSE(adapter.request_config(dt::ServiceCategory::AC_DER, required_functions).has_value());
        }
    }
}

// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Vedecom Contributors to EVerest
#include <catch2/catch_test_macros.hpp>

#include <iso15118/d20/ac_der_control.hpp>

#include <optional>

namespace d20 = iso15118::d20;
namespace dt = iso15118::message_20::datatypes;

namespace {

float value_of(const dt::RationalNumber& value) {
    return dt::from_RationalNumber(value);
}

d20::AcDerControlContext make_ac_der_context(const dt::DERControlFunctions& functions) {
    return {dt::ServiceCategory::AC_DER, dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc, functions};
}

} // namespace

SCENARIO("AC DER SECC control provider") {
    GIVEN("valid fresh SECC snapshots and a selected AC_DER service") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto config = provider->get_ac_der_control_config(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE(config.has_value());
        REQUIRE(config->cpd_control.active_power_support.has_value());
        REQUIRE(config->cpd_control.active_power_support->volt_watt.has_value());
        REQUIRE(config->cpd_control.active_power_support->under_frequency_watt.has_value());
        REQUIRE(config->cpd_control.active_power_support->over_frequency_watt.has_value());
        REQUIRE(config->cpd_control.reactive_power_support.has_value());
        REQUIRE(config->cpd_control.reactive_power_support->volt_var.has_value());
        REQUIRE(config->cpd_control.reactive_power_support->watt_var.has_value());
        REQUIRE(config->cpd_control.reactive_power_support->watt_cos_phi.has_value());
        REQUIRE(config->cpd_control.overvoltage_fault_ride_through.has_value());
        REQUIRE(config->cpd_control.undervoltage_fault_ride_through.has_value());
        REQUIRE(config->cpd_control.zero_current.has_value());
        REQUIRE(config->cpd_control.maximum_level_dc_injection.has_value());
        REQUIRE(value_of(config->cpd_control.active_power_support->volt_watt->u_start) == 241.0f);
        REQUIRE(value_of(*config->cpd_control.maximum_level_dc_injection) == 0.5f);
        REQUIRE(value_of(config->dso_q_setpoint.value) == 700.0f);
        REQUIRE(value_of(config->dso_cos_phi_setpoint.value) == 0.95f);
    }

    GIVEN("a stale grid policy snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.runtime_state.grid_policy_fresh = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto config = provider->get_ac_der_control_config(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("a stale DSO control snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.runtime_state.dso_control_fresh = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto config = provider->get_ac_der_control_config(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("AC DER is disabled by application runtime state") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.runtime_state.ac_der_enabled = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto config = provider->get_ac_der_control_config(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("an invalid grid policy snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.grid_policy.valid = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto config = provider->get_ac_der_control_config(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("an invalid DSO control snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.dso_control.valid = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto config = provider->get_ac_der_control_config(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("an invalid EVSE capability snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.evse_capability.valid = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto config = provider->get_ac_der_control_config(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("a selected AC_DER control function that is not supported by the EVSE capability snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto selected_functions = snapshots.evse_capability.supported_control_functions;
        snapshots.evse_capability.supported_control_functions.volt_watt = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto config = provider->get_ac_der_control_config(make_ac_der_context(selected_functions));

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("a non-AC_DER service context") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto config = provider->get_ac_der_control_config(
            {dt::ServiceCategory::AC_BPT, dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc,
             snapshots.evse_capability.supported_control_functions});

        REQUIRE_FALSE(config.has_value());
    }

    GIVEN("an AC_DER context without negotiated DER control functions") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto config = provider->get_ac_der_control_config({dt::ServiceCategory::AC_DER, dt::ControlMode::Dynamic,
                                                                 dt::MobilityNeedsMode::ProvidedByEvcc, std::nullopt});

        REQUIRE_FALSE(config.has_value());
    }
}

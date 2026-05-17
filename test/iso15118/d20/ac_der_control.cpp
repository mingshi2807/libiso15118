// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Vedecom Contributors to EVerest
#include <catch2/catch_test_macros.hpp>

#include <iso15118/d20/ac_der_control.hpp>

#include <optional>
#include <string>

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

        const auto context = make_ac_der_context(snapshots.evse_capability.supported_control_functions);
        const auto result = provider->get_ac_der_control_result(context);
        const auto config = provider->get_ac_der_control_config(context);

        REQUIRE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::None);
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

        const auto result = provider->get_ac_der_control_result(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::StaleGridPolicy);
    }

    GIVEN("a stale DSO control snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.runtime_state.dso_control_fresh = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::StaleDsoControl);
    }

    GIVEN("AC DER is disabled by application runtime state") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.runtime_state.ac_der_enabled = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::AcDerDisabled);
    }

    GIVEN("an invalid grid policy snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.grid_policy.valid = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::InvalidGridPolicy);
    }

    GIVEN("an invalid DSO control snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.dso_control.valid = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::InvalidDsoControl);
    }

    GIVEN("an invalid EVSE capability snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.evse_capability.valid = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::InvalidEvseCapability);
    }

    GIVEN("a selected AC_DER control function that is not supported by the EVSE capability snapshot") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto selected_functions = snapshots.evse_capability.supported_control_functions;
        snapshots.evse_capability.supported_control_functions.volt_watt = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(make_ac_der_context(selected_functions));

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::MissingSupportedMandatoryControlFunctions);
    }

    GIVEN("an EVSE capability snapshot without mandatory under-voltage fault ride-through") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.evse_capability.supported_control_functions.under_voltage_fault_ride_through = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(make_ac_der_context(
            d20::make_default_ac_der_secc_control_snapshots().evse_capability.supported_control_functions));

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::MissingSupportedMandatoryControlFunctions);
    }

    GIVEN("a non-AC_DER service context") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(
            {dt::ServiceCategory::AC_BPT, dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc,
             snapshots.evse_capability.supported_control_functions});

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::NonAcDerServiceSelected);
    }

    GIVEN("an AC_DER context without negotiated DER control functions") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result({dt::ServiceCategory::AC_DER, dt::ControlMode::Dynamic,
                                                                 dt::MobilityNeedsMode::ProvidedByEvcc, std::nullopt});

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::MissingSelectedControlFunctions);
    }

    GIVEN("an AC_DER context with an incomplete mandatory selected control bitmap") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto selected_functions = snapshots.evse_capability.supported_control_functions;
        selected_functions.zero_current = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(make_ac_der_context(selected_functions));

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::MissingSelectedMandatoryControlFunctions);
    }

    GIVEN("an AC_DER context without selected under-voltage fault ride-through") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto selected_functions = snapshots.evse_capability.supported_control_functions;
        selected_functions.under_voltage_fault_ride_through = false;
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(make_ac_der_context(selected_functions));

        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::MissingSelectedMandatoryControlFunctions);
    }

    GIVEN("a static AC DER provider result is requested") {
        auto provider = d20::make_static_ac_der_control_provider(d20::make_default_ac_der_control_config());

        const auto ac_der_result =
            provider->get_ac_der_control_result({dt::ServiceCategory::AC_DER, dt::ControlMode::Dynamic,
                                                 dt::MobilityNeedsMode::ProvidedByEvcc, std::nullopt});
        const auto ac_bpt_result =
            provider->get_ac_der_control_result({dt::ServiceCategory::AC_BPT, dt::ControlMode::Dynamic,
                                                 dt::MobilityNeedsMode::ProvidedByEvcc, std::nullopt});

        REQUIRE(ac_der_result.config.has_value());
        REQUIRE(ac_der_result.failure_reason == d20::AcDerControlFailureReason::None);
        REQUIRE_FALSE(ac_bpt_result.config.has_value());
        REQUIRE(ac_bpt_result.failure_reason == d20::AcDerControlFailureReason::NonAcDerServiceSelected);
    }

    GIVEN("AC DER control failure reasons are formatted for application diagnostics") {
        REQUIRE(d20::ac_der_control_failure_reason_to_string(d20::AcDerControlFailureReason::None) ==
                std::string("none"));
        REQUIRE(d20::ac_der_control_failure_reason_to_string(d20::AcDerControlFailureReason::StaleGridPolicy) ==
                std::string("stale_grid_policy"));
        REQUIRE(d20::ac_der_control_failure_reason_to_string(
                    d20::AcDerControlFailureReason::MissingSupportedMandatoryControlFunctions) ==
                std::string("missing_supported_mandatory_control_functions"));
    }
}

// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Vedecom Contributors to EVerest
#include <catch2/catch_test_macros.hpp>

#include <iso15118/d20/ac_der_control.hpp>

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace d20 = iso15118::d20;
namespace dt = iso15118::message_20::datatypes;

namespace {

float value_of(const dt::RationalNumber& value) {
    return dt::from_RationalNumber(value);
}

d20::AcDerControlContext make_ac_der_context(const dt::DERControlFunctions& functions) {
    return {dt::ServiceCategory::AC_DER, dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc, functions};
}

using ControlMutator = std::function<void(dt::DERControlFunctions&)>;

std::vector<ControlMutator> mandatory_control_mutators() {
    return {
        [](dt::DERControlFunctions& controls) { controls.volt_watt = false; },
        [](dt::DERControlFunctions& controls) { controls.dso_q_setpoint_provision = false; },
        [](dt::DERControlFunctions& controls) { controls.dso_cos_phi_setpoint_provision = false; },
        [](dt::DERControlFunctions& controls) { controls.dc_injection_restriction = false; },
        [](dt::DERControlFunctions& controls) { controls.under_frequency_watt = false; },
        [](dt::DERControlFunctions& controls) { controls.over_frequency_watt = false; },
        [](dt::DERControlFunctions& controls) { controls.volt_var = false; },
        [](dt::DERControlFunctions& controls) { controls.watt_var = false; },
        [](dt::DERControlFunctions& controls) { controls.watt_cos_phi = false; },
        [](dt::DERControlFunctions& controls) { controls.over_voltage_fault_ride_through = false; },
        [](dt::DERControlFunctions& controls) { controls.under_voltage_fault_ride_through = false; },
        [](dt::DERControlFunctions& controls) { controls.zero_current = false; },
    };
}

} // namespace

SCENARIO("AC DER SECC control provider") {
    GIVEN("valid generic SECC snapshots and a selected AC_DER service") {
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
        REQUIRE(value_of(config->cpd_control.active_power_support->volt_watt->u_start) == 230.0f);
        REQUIRE(value_of(*config->cpd_control.maximum_level_dc_injection) == 0.0f);
        REQUIRE(value_of(config->dso_q_setpoint.value) == 0.0f);
        REQUIRE(value_of(config->dso_cos_phi_setpoint.value) == 1.0f);
    }

    GIVEN("AC_DER_IEC Dynamic EIM profile snapshots and a selected AC_DER service") {
        auto snapshots = d20::make_ac_der_iec_dynamic_eim_profile_snapshots();
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto context = make_ac_der_context(snapshots.evse_capability.supported_control_functions);
        const auto result = provider->get_ac_der_control_result(context);
        const auto config = provider->get_ac_der_control_config(context);

        REQUIRE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::None);
        REQUIRE(config.has_value());
        REQUIRE(config->cpd_control.active_power_support.has_value());
        REQUIRE(config->cpd_control.active_power_support->volt_watt.has_value());
        REQUIRE(config->cpd_control.maximum_level_dc_injection.has_value());
        REQUIRE(value_of(config->cpd_control.active_power_support->volt_watt->u_start) == 241.0f);
        REQUIRE(value_of(config->cpd_control.active_power_support->volt_watt->u_stop) == 253.0f);
        REQUIRE(value_of(*config->cpd_control.maximum_level_dc_injection) == 0.5f);
        REQUIRE(value_of(config->dso_q_setpoint.value) == 700.0f);
        REQUIRE(value_of(config->dso_cos_phi_setpoint.value) == 0.95f);
        REQUIRE(config->dso_cos_phi_setpoint.excitation == dt::DERPowerFactorExcitation::UnderExcited);
    }

    GIVEN("default AC_DER IEC capability exposes all mandatory control bits and bitmap values") {
        const auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        const auto& controls = snapshots.evse_capability.supported_control_functions;

        REQUIRE(d20::has_required_ac_der_control_functions(controls));
        REQUIRE(controls.standard_bitmap == 0x3f);
        REQUIRE(controls.extended_bitmap == 0x3f);
        REQUIRE(controls.volt_watt);
        REQUIRE(controls.dso_q_setpoint_provision);
        REQUIRE(controls.dso_cos_phi_setpoint_provision);
        REQUIRE(controls.dc_injection_restriction);
        REQUIRE(controls.under_frequency_watt);
        REQUIRE(controls.over_frequency_watt);
        REQUIRE(controls.volt_var);
        REQUIRE(controls.watt_var);
        REQUIRE(controls.watt_cos_phi);
        REQUIRE(controls.over_voltage_fault_ride_through);
        REQUIRE(controls.under_voltage_fault_ride_through);
        REQUIRE(controls.zero_current);
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
        REQUIRE(d20::validate_ac_der_secc_control_snapshots(snapshots) ==
                d20::AcDerControlFailureReason::AcDerDisabled);
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

    GIVEN("a grid policy snapshot with reversed VoltWatt voltage thresholds") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.grid_policy.volt_watt_start_voltage = {253, 0};
        snapshots.grid_policy.volt_watt_stop_voltage = {241, 0};
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(d20::validate_ac_der_grid_policy_snapshot(snapshots.grid_policy));
        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::InvalidGridPolicy);
    }

    GIVEN("a grid policy snapshot with reversed under-frequency thresholds") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.grid_policy.under_frequency_watt_start_hz = {495, -1};
        snapshots.grid_policy.under_frequency_watt_stop_hz = {498, -1};
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(d20::validate_ac_der_grid_policy_snapshot(snapshots.grid_policy));
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

    GIVEN("a DSO control snapshot with invalid cos phi") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.dso_control.cos_phi_setpoint.value = {101, -2};
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(d20::validate_ac_der_dso_control_snapshot(snapshots.dso_control));
        REQUIRE_FALSE(result.config.has_value());
        REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::InvalidDsoControl);
    }

    GIVEN("a DSO control snapshot with negative response time") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        snapshots.dso_control.q_setpoint.step_response_time_constant_reactive_power = {-1, 0};
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto result = provider->get_ac_der_control_result(
            make_ac_der_context(snapshots.evse_capability.supported_control_functions));

        REQUIRE_FALSE(d20::validate_ac_der_dso_control_snapshot(snapshots.dso_control));
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
        REQUIRE(d20::validate_ac_der_secc_control_snapshots(snapshots) ==
                d20::AcDerControlFailureReason::MissingSupportedMandatoryControlFunctions);
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

    GIVEN("an AC_DER context outside the Dynamic EVCC-provided contract") {
        auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        const auto scheduled_result = provider->get_ac_der_control_result(
            {dt::ServiceCategory::AC_DER, dt::ControlMode::Scheduled, dt::MobilityNeedsMode::ProvidedByEvcc,
             snapshots.evse_capability.supported_control_functions});
        const auto secc_mobility_result = provider->get_ac_der_control_result(
            {dt::ServiceCategory::AC_DER, dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedBySecc,
             snapshots.evse_capability.supported_control_functions});

        REQUIRE_FALSE(scheduled_result.config.has_value());
        REQUIRE(scheduled_result.failure_reason == d20::AcDerControlFailureReason::UnsupportedControlMode);
        REQUIRE_FALSE(secc_mobility_result.config.has_value());
        REQUIRE(secc_mobility_result.failure_reason == d20::AcDerControlFailureReason::UnsupportedMobilityNeedsMode);
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

    GIVEN("any missing selected mandatory control function rejects AC_DER") {
        const auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        const auto provider = d20::make_secc_ac_der_control_provider(snapshots);

        for (const auto& drop_control : mandatory_control_mutators()) {
            auto selected_functions = snapshots.evse_capability.supported_control_functions;
            drop_control(selected_functions);

            const auto result = provider->get_ac_der_control_result(make_ac_der_context(selected_functions));

            REQUIRE_FALSE(result.config.has_value());
            REQUIRE(result.failure_reason == d20::AcDerControlFailureReason::MissingSelectedMandatoryControlFunctions);
        }
    }

    GIVEN("any missing supported mandatory control function rejects SECC snapshots at preflight") {
        for (const auto& drop_control : mandatory_control_mutators()) {
            auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
            drop_control(snapshots.evse_capability.supported_control_functions);

            REQUIRE_FALSE(
                d20::has_required_ac_der_control_functions(snapshots.evse_capability.supported_control_functions));
            REQUIRE(d20::validate_ac_der_secc_control_snapshots(snapshots) ==
                    d20::AcDerControlFailureReason::MissingSupportedMandatoryControlFunctions);
        }
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

    GIVEN("a default AC DER control config is production-valid for mandatory controls") {
        const auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        const auto config = d20::make_default_ac_der_control_config();

        REQUIRE(d20::validate_ac_der_control_config(config, snapshots.evse_capability.supported_control_functions));
    }

    GIVEN("an AC DER control config with reversed VoltWatt thresholds") {
        const auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto config = d20::make_default_ac_der_control_config();
        config.cpd_control.active_power_support->volt_watt->u_start = {253, 0};
        config.cpd_control.active_power_support->volt_watt->u_stop = {241, 0};

        REQUIRE_FALSE(
            d20::validate_ac_der_control_config(config, snapshots.evse_capability.supported_control_functions));
    }

    GIVEN("an AC DER control config with a non-monotonic VoltVar curve") {
        const auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto config = d20::make_default_ac_der_control_config();
        auto& curve = *config.cpd_control.reactive_power_support->volt_var;
        curve.curve_data_points[1].x_value = curve.curve_data_points[0].x_value;

        REQUIRE_FALSE(
            d20::validate_ac_der_control_config(config, snapshots.evse_capability.supported_control_functions));
    }

    GIVEN("an AC DER control config without a ZeroCurrent voltage limit") {
        const auto snapshots = d20::make_default_ac_der_secc_control_snapshots();
        auto config = d20::make_default_ac_der_control_config();
        config.cpd_control.zero_current->over_voltage_limit = std::nullopt;
        config.cpd_control.zero_current->under_voltage_limit = std::nullopt;

        REQUIRE_FALSE(
            d20::validate_ac_der_control_config(config, snapshots.evse_capability.supported_control_functions));
    }

    GIVEN("AC DER control failure reasons are formatted for application diagnostics") {
        REQUIRE(d20::ac_der_control_failure_reason_to_string(d20::AcDerControlFailureReason::None) ==
                std::string("none"));
        REQUIRE(d20::ac_der_control_failure_reason_to_string(d20::AcDerControlFailureReason::StaleGridPolicy) ==
                std::string("stale_grid_policy"));
        REQUIRE(d20::ac_der_control_failure_reason_to_string(d20::AcDerControlFailureReason::UnsupportedControlMode) ==
                std::string("unsupported_control_mode"));
        REQUIRE(d20::ac_der_control_failure_reason_to_string(
                    d20::AcDerControlFailureReason::UnsupportedMobilityNeedsMode) ==
                std::string("unsupported_mobility_needs_mode"));
        REQUIRE(d20::ac_der_control_failure_reason_to_string(
                    d20::AcDerControlFailureReason::MissingSupportedMandatoryControlFunctions) ==
                std::string("missing_supported_mandatory_control_functions"));
    }
}

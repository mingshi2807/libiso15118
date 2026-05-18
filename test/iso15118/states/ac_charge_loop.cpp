// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#include <catch2/catch_test_macros.hpp>

#include <iso15118/detail/d20/state/ac_charge_loop.hpp>

#include <iso15118/d20/config.hpp>

#include <cbv2g/iso_20/iso20_AC_Datatypes.h>

#include <memory>
#include <utility>

using namespace iso15118;

namespace dt = message_20::datatypes;

using Scheduled_AC_Req = dt::Scheduled_AC_CLReqControlMode;
using Scheduled_BPT_AC_Req = dt::BPT_Scheduled_AC_CLReqControlMode;
using Scheduled_DER_AC_Req = dt::DER_Scheduled_AC_CLReqControlMode;
using Dynamic_AC_Req = dt::Dynamic_AC_CLReqControlMode;
using Dynamic_BPT_AC_Req = dt::BPT_Dynamic_AC_CLReqControlMode;
using Dynamic_DER_AC_Req = dt::DER_Dynamic_AC_CLReqControlMode;

using Scheduled_AC_Res = dt::Scheduled_AC_CLResControlMode;
using Scheduled_BPT_AC_Res = dt::BPT_Scheduled_AC_CLResControlMode;
using Scheduled_DER_AC_Res = dt::DER_Scheduled_AC_CLResControlMode;
using Dynamic_AC_Res = dt::Dynamic_AC_CLResControlMode;
using Dynamic_BPT_AC_Res = dt::BPT_Dynamic_AC_CLResControlMode;
using Dynamic_DER_AC_Res = dt::DER_Dynamic_AC_CLResControlMode;

namespace {
dt::DERControlFunctions get_mandatory_der_control_functions() {
    dt::DERControlFunctions der_control_functions;
    der_control_functions.volt_watt = true;
    der_control_functions.dso_q_setpoint_provision = true;
    der_control_functions.dso_cos_phi_setpoint_provision = true;
    der_control_functions.dc_injection_restriction = true;
    der_control_functions.under_frequency_watt = true;
    der_control_functions.over_frequency_watt = true;
    der_control_functions.volt_var = true;
    der_control_functions.watt_var = true;
    der_control_functions.watt_cos_phi = true;
    der_control_functions.over_voltage_fault_ride_through = true;
    der_control_functions.under_voltage_fault_ride_through = true;
    der_control_functions.zero_current = true;
    return der_control_functions;
}

class RecordingAcDerControlProvider : public d20::IAcDerControlProvider {
public:
    explicit RecordingAcDerControlProvider(d20::AcDerControlConfig config_) : config(std::move(config_)) {
    }

    std::optional<d20::AcDerControlConfig>
    get_ac_der_control_config(const d20::AcDerControlContext& context) const override {
        calls++;
        last_context = context;
        return config;
    }

    d20::AcDerControlConfig config;
    mutable int calls{0};
    mutable std::optional<d20::AcDerControlContext> last_context;
};

class UnavailableAcDerControlProvider : public d20::IAcDerControlProvider {
public:
    std::optional<d20::AcDerControlConfig>
    get_ac_der_control_config(const d20::AcDerControlContext& context) const override {
        (void)context;
        return std::nullopt;
    }
};

class RejectingAcDerControlProvider : public d20::IAcDerControlProvider {
public:
    explicit RejectingAcDerControlProvider(d20::AcDerControlFailureReason reason_) : reason(reason_) {
    }

    std::optional<d20::AcDerControlConfig>
    get_ac_der_control_config(const d20::AcDerControlContext& context) const override {
        (void)context;
        return std::nullopt;
    }

    d20::AcDerControlResult get_ac_der_control_result(const d20::AcDerControlContext& context) const override {
        (void)context;
        return {std::nullopt, reason};
    }

    d20::AcDerControlFailureReason reason;
};

bool same_rational(const iso20_ac_RationalNumberType& actual, const dt::RationalNumber& expected) {
    return actual.Value == expected.value and actual.Exponent == expected.exponent;
}
} // namespace

SCENARIO("AC charge loop state handling") {

    const auto evse_id = std::string("everest se");
    const std::vector<dt::ServiceCategory> supported_energy_services = {dt::ServiceCategory::AC,
                                                                        dt::ServiceCategory::AC_BPT};
    const auto cert_install{false};
    const std::vector<dt::Authorization> auth_services = {dt::Authorization::EIM};
    const std::vector<uint16_t> vas_services{};

    d20::DcTransferLimits dc_limits;
    d20::DcTransferLimits powersupply_limits;
    d20::AcTransferLimits ac_limits;
    ac_limits.charge_power = {{22, 3}, {10, 0}};
    ac_limits.nominal_frequency = {50, 0};

    auto& discharge_limits = ac_limits.discharge_power.emplace();
    discharge_limits.max = {11, 3};
    discharge_limits.min = {10, 0};

    const std::vector<d20::ControlMobilityNeedsModes> control_mobility_modes = {
        {dt::ControlMode::Scheduled, dt::MobilityNeedsMode::ProvidedByEvcc},
        {dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc},
        {dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedBySecc}};

    const d20::EvseSetupConfig evse_setup{
        evse_id,   supported_energy_services, auth_services, vas_services, cert_install, dc_limits,
        ac_limits, control_mobility_modes,    std::nullopt,  std::nullopt, std::nullopt, powersupply_limits};

    GIVEN("Bad case - Unknown session") {
        d20::Session session = d20::Session();
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Scheduled_AC_Req>();
        req_control_mode.present_active_power = {0, 0};

        req.meter_info_requested = false;

        const auto res = d20::state::handle_request(req, d20::Session(), false, false, 50, d20::AcTargetPower(),
                                                    d20::AcPresentPower(), d20::UpdateDynamicModeParameters());

        THEN("ResponseCode: FAILED_UnknownSession, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED_UnknownSession);
            REQUIRE(res.status.has_value() == false);
            REQUIRE(res.meter_info.has_value() == false);
            REQUIRE(res.receipt.has_value() == false);
            REQUIRE(res.target_frequency.has_value() == false);
            REQUIRE(std::holds_alternative<Scheduled_AC_Res>(res.control_mode));
        }
    }

    GIVEN("Bad case - false energy mode") {

        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_BPT, dt::AcConnector::ThreePhase, dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Scheduled_AC_Req>();
        req_control_mode.present_active_power = {0, 0};

        req.meter_info_requested = false;

        const auto res = d20::state::handle_request(req, d20::Session(), false, false, 50, d20::AcTargetPower(),
                                                    d20::AcPresentPower(), d20::UpdateDynamicModeParameters());

        THEN("ResponseCode: FAILED_UnknownSession, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED_UnknownSession);
            REQUIRE(res.status.has_value() == false);
            REQUIRE(res.meter_info.has_value() == false);
            REQUIRE(res.receipt.has_value() == false);
            REQUIRE(res.target_frequency.has_value() == false);
            REQUIRE(std::holds_alternative<Scheduled_AC_Res>(res.control_mode));
        }
    }

    GIVEN("Bad case - false control mode") {

        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, 230);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Scheduled_AC_Req>();
        req_control_mode.present_active_power = {0, 0};

        req.meter_info_requested = false;

        const auto res = d20::state::handle_request(req, d20::Session(), false, false, 50, d20::AcTargetPower(),
                                                    d20::AcPresentPower(), d20::UpdateDynamicModeParameters());

        THEN("ResponseCode: FAILED_UnknownSession, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED_UnknownSession);
            REQUIRE(res.status.has_value() == false);
            REQUIRE(res.meter_info.has_value() == false);
            REQUIRE(res.receipt.has_value() == false);
            REQUIRE(res.target_frequency.has_value() == false);
            REQUIRE(std::holds_alternative<Scheduled_AC_Res>(res.control_mode));
        }
    }

    GIVEN("Good case - AC scheduled mode") {

        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC, dt::AcConnector::ThreePhase, dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, 230);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Scheduled_AC_Req>();
        req_control_mode.present_active_power = {11, 3};

        req.meter_info_requested = false;

        const auto ac_target_power = d20::AcTargetPower{};
        auto ac_present_power = d20::AcPresentPower{};
        ac_present_power.present_active_power = {11, 3};

        const auto res = d20::state::handle_request(req, session, false, false, 50, ac_target_power, ac_present_power,
                                                    d20::UpdateDynamicModeParameters());

        THEN("ResponseCode: OK, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
            REQUIRE(res.status.has_value() == false);
            REQUIRE(res.meter_info.has_value() == false);
            REQUIRE(res.receipt.has_value() == false);
            REQUIRE(dt::from_RationalNumber(res.target_frequency.value_or(dt::RationalNumber{0, 0})) == 50.0f);
            REQUIRE(std::holds_alternative<Scheduled_AC_Res>(res.control_mode));

            const auto& res_control_mode = std::get<Scheduled_AC_Res>(res.control_mode);
            REQUIRE(dt::from_RationalNumber(res_control_mode.present_active_power.value_or(dt::RationalNumber{0, 0})) ==
                    11000.0f);
        }
    }

    GIVEN("Good case - AC_BPT scheduled mode") {

        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_BPT, dt::AcConnector::ThreePhase, dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Scheduled_BPT_AC_Req>();
        req_control_mode.present_active_power = {0, 0};

        req.meter_info_requested = false;

        const auto ac_target_power = d20::AcTargetPower{};
        auto ac_present_power = d20::AcPresentPower{};
        ac_present_power.present_active_power = {11, 3};

        const auto res = d20::state::handle_request(req, session, false, false, 50, ac_target_power, ac_present_power,
                                                    d20::UpdateDynamicModeParameters());

        THEN("ResponseCode: OK, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
            REQUIRE(res.status.has_value() == false);
            REQUIRE(res.meter_info.has_value() == false);
            REQUIRE(res.receipt.has_value() == false);
            REQUIRE(dt::from_RationalNumber(res.target_frequency.value_or(dt::RationalNumber{0, 0})) == 50.0f);
            REQUIRE(std::holds_alternative<Scheduled_BPT_AC_Res>(res.control_mode));

            const auto& res_control_mode = std::get<Scheduled_BPT_AC_Res>(res.control_mode);
            REQUIRE(dt::from_RationalNumber(res_control_mode.present_active_power.value_or(dt::RationalNumber{0, 0})) ==
                    11000.0f);
        }
    }

    GIVEN("Good case - AC dynamic mode") {

        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, 230);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Dynamic_AC_Req>();
        req_control_mode.present_active_power = {11, 3};
        req_control_mode.max_charge_power = {11, 3};
        req_control_mode.min_charge_power = {4, 0};
        req_control_mode.present_reactive_power = {10, 0};

        req.meter_info_requested = false;

        auto ac_target_power = d20::AcTargetPower{};
        ac_target_power.target_active_power = {11, 3};
        auto ac_present_power = d20::AcPresentPower{};
        ac_present_power.present_active_power = {11, 3};
        RecordingAcDerControlProvider der_provider(d20::make_default_ac_der_control_config());

        const auto res = d20::state::handle_request(req, session, false, false, 50, ac_limits, ac_target_power,
                                                    ac_present_power, d20::UpdateDynamicModeParameters(), der_provider);

        THEN("ResponseCode: OK, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
            REQUIRE(der_provider.calls == 0);
            REQUIRE(res.status.has_value() == false);
            REQUIRE(res.meter_info.has_value() == false);
            REQUIRE(res.receipt.has_value() == false);
            REQUIRE(dt::from_RationalNumber(res.target_frequency.value_or(dt::RationalNumber{0, 0})) == 50.0f);
            REQUIRE(std::holds_alternative<Dynamic_AC_Res>(res.control_mode));

            const auto& res_control_mode = std::get<Dynamic_AC_Res>(res.control_mode);
            REQUIRE(dt::from_RationalNumber(res_control_mode.present_active_power.value_or(dt::RationalNumber{0, 0})) ==
                    11000.0f);
            REQUIRE(dt::from_RationalNumber(res_control_mode.target_active_power) == 11000.0f);
        }
    }

    GIVEN("Good case - AC_BPT dynamic mode") {
        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_BPT, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Dynamic_BPT_AC_Req>();
        req_control_mode.present_active_power = {11, 3};
        req_control_mode.max_charge_power = {11, 3};
        req_control_mode.min_charge_power = {4, 0};
        req_control_mode.present_reactive_power = {10, 0};

        req.meter_info_requested = false;

        auto ac_target_power = d20::AcTargetPower{};
        ac_target_power.target_active_power = {11, 3};
        auto ac_present_power = d20::AcPresentPower{};
        ac_present_power.present_active_power = {11, 3};

        const auto res = d20::state::handle_request(req, session, false, false, 50, ac_target_power, ac_present_power,
                                                    d20::UpdateDynamicModeParameters());

        THEN("ResponseCode: OK, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
            REQUIRE(res.status.has_value() == false);
            REQUIRE(res.meter_info.has_value() == false);
            REQUIRE(res.receipt.has_value() == false);
            REQUIRE(dt::from_RationalNumber(res.target_frequency.value_or(dt::RationalNumber{0, 0})) == 50.0f);
            REQUIRE(std::holds_alternative<Dynamic_BPT_AC_Res>(res.control_mode));

            const auto& res_control_mode = std::get<Dynamic_BPT_AC_Res>(res.control_mode);
            REQUIRE(dt::from_RationalNumber(res_control_mode.present_active_power.value_or(dt::RationalNumber{0, 0})) ==
                    11000.0f);
            REQUIRE(dt::from_RationalNumber(res_control_mode.target_active_power) == 11000.0f);
        }
    }

    GIVEN("Bad case - DER dynamic control mode while AC_BPT service is selected") {
        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_BPT, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.control_mode.emplace<Dynamic_DER_AC_Req>();
        req.meter_info_requested = false;

        RecordingAcDerControlProvider der_provider(d20::make_default_ac_der_control_config());
        const auto res =
            d20::state::handle_request(req, session, false, false, 50, ac_limits, d20::AcTargetPower{},
                                       d20::AcPresentPower{}, d20::UpdateDynamicModeParameters(), der_provider);

        THEN("ResponseCode: FAILED and AC DER provider is not queried") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED);
            REQUIRE(der_provider.calls == 0);
        }
    }

    GIVEN("Bad case - BPT dynamic control mode while AC_DER service is selected") {
        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_DER, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive,
            get_mandatory_der_control_functions());

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Dynamic_BPT_AC_Req>();
        req_control_mode.present_active_power = {11, 3};
        req_control_mode.max_charge_power = {11, 3};
        req_control_mode.min_charge_power = {4, 0};
        req_control_mode.present_reactive_power = {10, 0};
        req.meter_info_requested = false;

        RecordingAcDerControlProvider der_provider(d20::make_default_ac_der_control_config());
        const auto res =
            d20::state::handle_request(req, session, false, false, 50, ac_limits, d20::AcTargetPower{},
                                       d20::AcPresentPower{}, d20::UpdateDynamicModeParameters(), der_provider);

        THEN("ResponseCode: FAILED and AC DER provider is not queried") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED);
            REQUIRE(der_provider.calls == 0);
        }
    }

    GIVEN("Good case - AC_DER dynamic mode") {
        const auto der_control_functions = get_mandatory_der_control_functions();

        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_DER, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive,
            der_control_functions);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Dynamic_DER_AC_Req>();
        req_control_mode.target_energy_request = {22, 3};
        req_control_mode.max_energy_request = {30, 3};
        req_control_mode.min_energy_request = {10, 3};
        req_control_mode.max_charge_power = {22, 3};
        req_control_mode.min_charge_power = {4, 0};
        req_control_mode.present_active_power = {11, 3};
        req_control_mode.present_reactive_power = {10, 0};
        req_control_mode.max_discharge_power = {11, 3};
        req_control_mode.min_discharge_power = {4, 0};
        req_control_mode.max_charge_reactive_power = {2, 3};
        req_control_mode.max_discharge_reactive_power = {2, 3};
        req_control_mode.grid_event_condition = 1;

        req.meter_info_requested = false;

        auto ac_target_power = d20::AcTargetPower{};
        ac_target_power.target_active_power = {11, 3};
        ac_target_power.target_reactive_power = {2, 3};
        auto ac_present_power = d20::AcPresentPower{};
        ac_present_power.present_active_power = {11, 3};
        auto ac_der_control_config = d20::make_default_ac_der_control_config();
        ac_der_control_config.dso_q_setpoint.value = {7, 2};
        ac_der_control_config.dso_cos_phi_setpoint.value = {95, -2};
        ac_der_control_config.dso_cos_phi_setpoint.excitation = dt::DERPowerFactorExcitation::UnderExcited;
        auto ac_der_control_provider = std::make_shared<RecordingAcDerControlProvider>(ac_der_control_config);
        auto evse_setup_with_provider = evse_setup;
        evse_setup_with_provider.supported_energy_services = {dt::ServiceCategory::AC_DER};
        evse_setup_with_provider.ac_der_control_provider = ac_der_control_provider;
        const auto session_config = d20::SessionConfig(evse_setup_with_provider);

        const auto res =
            d20::state::handle_request(req, session, false, false, 50, ac_limits, ac_target_power, ac_present_power,
                                       d20::UpdateDynamicModeParameters(), *session_config.ac_der_control_provider);

        THEN("ResponseCode: OK and DER control mode should be selected") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
            REQUIRE(ac_der_control_provider->calls == 1);
            REQUIRE(ac_der_control_provider->last_context.has_value());
            REQUIRE(ac_der_control_provider->last_context->selected_energy_service == dt::ServiceCategory::AC_DER);
            REQUIRE(ac_der_control_provider->last_context->selected_control_mode == dt::ControlMode::Dynamic);
            REQUIRE(std::holds_alternative<Dynamic_DER_AC_Res>(res.control_mode));

            const auto& res_control_mode = std::get<Dynamic_DER_AC_Res>(res.control_mode);
            REQUIRE(dt::from_RationalNumber(res_control_mode.target_active_power) == 11000.0f);
            REQUIRE(dt::from_RationalNumber(res_control_mode.max_charge_power) == 22000.0f);
            REQUIRE(dt::from_RationalNumber(res_control_mode.max_discharge_power) == 11000.0f);
            REQUIRE(res_control_mode.dso_q_setpoint.has_value());
            REQUIRE(dt::from_RationalNumber(res_control_mode.dso_q_setpoint->value) == 700.0f);
            REQUIRE(res_control_mode.dso_cos_phi_setpoint.has_value());
            REQUIRE(dt::from_RationalNumber(res_control_mode.dso_cos_phi_setpoint->value) == 0.95f);
            REQUIRE(res_control_mode.dso_cos_phi_setpoint->excitation == dt::DERPowerFactorExcitation::UnderExcited);
        }
    }

    GIVEN("Bad case - AC_DER dynamic mode but application provider has no AC DER config") {
        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_DER, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive,
            get_mandatory_der_control_functions());

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.control_mode.emplace<Dynamic_DER_AC_Req>();
        req.meter_info_requested = false;

        const UnavailableAcDerControlProvider provider;
        const auto res =
            d20::state::handle_request(req, session, false, false, 50, ac_limits, d20::AcTargetPower{},
                                       d20::AcPresentPower{}, d20::UpdateDynamicModeParameters(), provider);

        THEN("ResponseCode: FAILED") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED);
        }
    }

    GIVEN("Bad case - AC_DER dynamic mode but provider rejects ChargeLoop with stale grid policy") {
        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_DER, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive,
            get_mandatory_der_control_functions());

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.control_mode.emplace<Dynamic_DER_AC_Req>();
        req.meter_info_requested = false;

        const RejectingAcDerControlProvider provider(d20::AcDerControlFailureReason::StaleGridPolicy);
        const auto result = d20::state::handle_request_with_diagnostics(req, session, false, false, 50, ac_limits,
                                                                        d20::AcTargetPower{}, d20::AcPresentPower{},
                                                                        d20::UpdateDynamicModeParameters(), provider);

        THEN("ResponseCode: FAILED and provider reason is preserved") {
            REQUIRE(result.response.response_code == dt::ResponseCode::FAILED);
            REQUIRE(result.ac_der_failure_reason == d20::AcDerControlFailureReason::StaleGridPolicy);
        }
    }

    GIVEN("Bad case - AC_DER dynamic mode with incomplete mandatory controls") {
        dt::DERControlFunctions der_control_functions;
        der_control_functions.volt_watt = true;

        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_DER, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive,
            der_control_functions);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.control_mode.emplace<Dynamic_DER_AC_Req>();
        req.meter_info_requested = false;

        const auto res = d20::state::handle_request(req, session, false, false, 50, ac_limits, d20::AcTargetPower{},
                                                    d20::AcPresentPower{}, d20::UpdateDynamicModeParameters());

        THEN("ResponseCode: FAILED") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED);
        }
    }

    GIVEN("Bad case - AC_DER dynamic mode without selected under-voltage fault ride-through") {
        auto der_control_functions = get_mandatory_der_control_functions();
        der_control_functions.under_voltage_fault_ride_through = false;

        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_DER, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive,
            der_control_functions);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.control_mode.emplace<Dynamic_DER_AC_Req>();
        req.meter_info_requested = false;

        const auto result = d20::state::handle_request_with_diagnostics(
            req, session, false, false, 50, ac_limits, d20::AcTargetPower{}, d20::AcPresentPower{},
            d20::UpdateDynamicModeParameters(),
            *d20::make_static_ac_der_control_provider(d20::make_default_ac_der_control_config()));

        THEN("ResponseCode: FAILED and selected bitmap reason is preserved") {
            REQUIRE(result.response.response_code == dt::ResponseCode::FAILED);
            REQUIRE(result.ac_der_failure_reason ==
                    d20::AcDerControlFailureReason::MissingSelectedMandatoryControlFunctions);
        }
    }

    GIVEN("Bad case - AC_DER scheduled mode is outside current SECC provider scope") {
        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_DER, dt::AcConnector::ThreePhase, dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive,
            get_mandatory_der_control_functions());

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.control_mode.emplace<Scheduled_DER_AC_Req>();
        req.meter_info_requested = false;

        const auto provider = d20::make_secc_ac_der_control_provider(d20::make_default_ac_der_secc_control_snapshots());
        const auto result = d20::state::handle_request_with_diagnostics(req, session, false, false, 50, ac_limits,
                                                                        d20::AcTargetPower{}, d20::AcPresentPower{},
                                                                        d20::UpdateDynamicModeParameters(), *provider);

        THEN("ResponseCode: FAILED and unsupported mode reason is preserved") {
            REQUIRE(result.response.response_code == dt::ResponseCode::FAILED);
            REQUIRE(result.ac_der_failure_reason == d20::AcDerControlFailureReason::UnsupportedControlMode);
        }
    }

    GIVEN("Bad case - AC_DER dynamic mode with invalid DSO cos phi setpoint") {
        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_DER, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive,
            get_mandatory_der_control_functions());

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.control_mode.emplace<Dynamic_DER_AC_Req>();
        req.meter_info_requested = false;

        auto ac_der_control_config = d20::make_default_ac_der_control_config();
        ac_der_control_config.dso_cos_phi_setpoint.value = {101, -2};
        const auto provider = RecordingAcDerControlProvider(ac_der_control_config);

        const auto res =
            d20::state::handle_request(req, session, false, false, 50, ac_limits, d20::AcTargetPower{},
                                       d20::AcPresentPower{}, d20::UpdateDynamicModeParameters(), provider);

        THEN("ResponseCode: FAILED") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED);
        }
    }

    GIVEN("Good case - AC dynamic mode, mobility_needs_mode = 2") {

        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedBySecc, dt::Pricing::NoPricing, 230);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Dynamic_AC_Req>();
        req_control_mode.present_active_power = {11, 3};
        req_control_mode.max_charge_power = {11, 3};
        req_control_mode.min_charge_power = {4, 0};
        req_control_mode.present_reactive_power = {10, 0};

        req.meter_info_requested = false;

        auto ac_target_power = d20::AcTargetPower{};
        ac_target_power.target_active_power = {11, 3};
        auto ac_present_power = d20::AcPresentPower{};
        ac_present_power.present_active_power = {11, 3};

        const d20::UpdateDynamicModeParameters dynamic_parameters = {std::time(nullptr) + 40, std::nullopt, 95};

        const auto res = d20::state::handle_request(req, session, false, false, 50, ac_target_power, ac_present_power,
                                                    dynamic_parameters);

        THEN("ResponseCode: OK, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
            REQUIRE(res.status.has_value() == false);
            REQUIRE(res.meter_info.has_value() == false);
            REQUIRE(res.receipt.has_value() == false);
            REQUIRE(dt::from_RationalNumber(res.target_frequency.value_or(dt::RationalNumber{0, 0})) == 50.0f);
            REQUIRE(std::holds_alternative<Dynamic_AC_Res>(res.control_mode));

            const auto& res_control_mode = std::get<Dynamic_AC_Res>(res.control_mode);
            REQUIRE(dt::from_RationalNumber(res_control_mode.present_active_power.value_or(dt::RationalNumber{0, 0})) ==
                    11000.0f);
            REQUIRE(dt::from_RationalNumber(res_control_mode.target_active_power) == 11000.0f);

            REQUIRE(res_control_mode.departure_time.value_or(0) >= 39);
            REQUIRE(res_control_mode.minimum_soc.value_or(0) == 95);
            REQUIRE(res_control_mode.ack_max_delay.value_or(0) == 30);
        }
    }

    GIVEN("Good case - AC_BPT dynamic mode, mobility_needs_mode = 2") {
        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC_BPT, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedBySecc, dt::Pricing::NoPricing, dt::BptChannel::Unified,
            dt::GeneratorMode::GridFollowing, 230, dt::GridCodeIslandingDetectionMethod::Passive);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Dynamic_BPT_AC_Req>();
        req_control_mode.present_active_power = {11, 3};
        req_control_mode.max_charge_power = {11, 3};
        req_control_mode.min_charge_power = {4, 0};
        req_control_mode.present_reactive_power = {10, 0};

        req.meter_info_requested = false;

        auto ac_target_power = d20::AcTargetPower{};
        ac_target_power.target_active_power = {11, 3};
        auto ac_present_power = d20::AcPresentPower{};
        ac_present_power.present_active_power = {11, 3};

        const d20::UpdateDynamicModeParameters dynamic_parameters = {std::time(nullptr) + 40, std::nullopt, 95};

        const auto res = d20::state::handle_request(req, session, false, false, 50, ac_target_power, ac_present_power,
                                                    dynamic_parameters);

        THEN("ResponseCode: OK, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
            REQUIRE(res.status.has_value() == false);
            REQUIRE(res.meter_info.has_value() == false);
            REQUIRE(res.receipt.has_value() == false);
            REQUIRE(dt::from_RationalNumber(res.target_frequency.value_or(dt::RationalNumber{0, 0})) == 50.0f);
            REQUIRE(std::holds_alternative<Dynamic_BPT_AC_Res>(res.control_mode));

            const auto& res_control_mode = std::get<Dynamic_BPT_AC_Res>(res.control_mode);
            REQUIRE(dt::from_RationalNumber(res_control_mode.present_active_power.value_or(dt::RationalNumber{0, 0})) ==
                    11000.0f);
            REQUIRE(dt::from_RationalNumber(res_control_mode.target_active_power) == 11000.0f);

            REQUIRE(res_control_mode.departure_time.value_or(0) >= 39);
            REQUIRE(res_control_mode.minimum_soc.value_or(0) == 95);
            REQUIRE(res_control_mode.ack_max_delay.value_or(0) == 30);
        }
    }

    GIVEN("Good case - AC dynamic mode & pause from charger") {

        d20::SelectedServiceParameters service_parameters = d20::SelectedServiceParameters(
            dt::ServiceCategory::AC, dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic,
            dt::MobilityNeedsMode::ProvidedByEvcc, dt::Pricing::NoPricing, 230);

        d20::Session session = d20::Session(service_parameters);
        message_20::AC_ChargeLoopRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;

        auto& req_control_mode = req.control_mode.emplace<Dynamic_AC_Req>();
        req_control_mode.present_active_power = {11, 3};
        req_control_mode.max_charge_power = {11, 3};
        req_control_mode.min_charge_power = {4, 0};
        req_control_mode.present_reactive_power = {10, 0};

        req.meter_info_requested = false;

        auto ac_target_power = d20::AcTargetPower{};
        ac_target_power.target_active_power = {11, 3};
        auto ac_present_power = d20::AcPresentPower{};
        ac_present_power.present_active_power = {11, 3};

        const auto res = d20::state::handle_request(req, session, false, true, 50, ac_target_power, ac_present_power,
                                                    d20::UpdateDynamicModeParameters());

        THEN("ResponseCode: OK, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
            REQUIRE(res.meter_info.has_value() == false);
            REQUIRE(res.receipt.has_value() == false);
            REQUIRE(dt::from_RationalNumber(res.target_frequency.value_or(dt::RationalNumber{0, 0})) == 50.0f);
            REQUIRE(std::holds_alternative<Dynamic_AC_Res>(res.control_mode));

            const auto& res_control_mode = std::get<Dynamic_AC_Res>(res.control_mode);
            REQUIRE(dt::from_RationalNumber(res_control_mode.present_active_power.value_or(dt::RationalNumber{0, 0})) ==
                    11000.0f);
            REQUIRE(dt::from_RationalNumber(res_control_mode.target_active_power) == 11000.0f);

            REQUIRE(res.status.has_value() == true);
            REQUIRE(res.status.value().notification == dt::EvseNotification::Pause);
            REQUIRE(res.status.value().notification_max_delay == 60);
        }
    }
}

SCENARIO("AC charge loop response serialization") {
    GIVEN("A scheduled DER AC response with distinct L2 and L3 targets") {
        message_20::AC_ChargeLoopResponse res;
        res.response_code = dt::ResponseCode::OK;

        auto& control_mode = res.control_mode.emplace<Scheduled_DER_AC_Res>();
        control_mode.target_active_power = dt::RationalNumber{1, 0};
        control_mode.target_active_power_L2 = dt::RationalNumber{2, 0};
        control_mode.target_active_power_L3 = dt::RationalNumber{3, 0};
        control_mode.target_reactive_power = dt::RationalNumber{4, 0};
        control_mode.target_reactive_power_L2 = dt::RationalNumber{5, 0};
        control_mode.target_reactive_power_L3 = dt::RationalNumber{6, 0};
        control_mode.max_charge_power = dt::RationalNumber{22, 3};
        control_mode.max_discharge_power = dt::RationalNumber{11, 3};
        control_mode.dso_q_setpoint =
            dt::DSOQSetpoint{dt::RationalNumber{7, 2}, std::nullopt, std::nullopt, false, dt::RationalNumber{0, 0}};
        control_mode.dso_cos_phi_setpoint = dt::DSOCosPhiSetpoint{
            dt::RationalNumber{95, -2}, std::nullopt, std::nullopt, dt::DERPowerFactorExcitation::UnderExcited, false,
            dt::RationalNumber{0, 0}};

        iso20_ac_AC_ChargeLoopResType out;
        message_20::convert(res, out);

        THEN("The generated cbv2g structure preserves L3 values") {
            REQUIRE(out.DER_Scheduled_AC_CLResControlMode_isUsed);
            const auto& der = out.DER_Scheduled_AC_CLResControlMode;
            REQUIRE(der.EVSETargetActivePower_L2_isUsed);
            REQUIRE(der.EVSETargetActivePower_L3_isUsed);
            REQUIRE(same_rational(der.EVSETargetActivePower_L2, dt::RationalNumber{2, 0}));
            REQUIRE(same_rational(der.EVSETargetActivePower_L3, dt::RationalNumber{3, 0}));
            REQUIRE(der.EVSETargetReactivePower_L2_isUsed);
            REQUIRE(der.EVSETargetReactivePower_L3_isUsed);
            REQUIRE(same_rational(der.EVSETargetReactivePower_L2, dt::RationalNumber{5, 0}));
            REQUIRE(same_rational(der.EVSETargetReactivePower_L3, dt::RationalNumber{6, 0}));
            REQUIRE(der.DSOQSetpoint_isUsed);
            REQUIRE(der.DSOCosPhiSetpoint_isUsed);
        }
    }

    GIVEN("A dynamic DER AC response with DSO setpoints") {
        message_20::AC_ChargeLoopResponse res;
        res.response_code = dt::ResponseCode::OK;

        auto& control_mode = res.control_mode.emplace<Dynamic_DER_AC_Res>();
        control_mode.target_active_power = dt::RationalNumber{1, 0};
        control_mode.target_active_power_L2 = dt::RationalNumber{2, 0};
        control_mode.target_active_power_L3 = dt::RationalNumber{3, 0};
        control_mode.max_charge_power = dt::RationalNumber{22, 3};
        control_mode.max_discharge_power = dt::RationalNumber{11, 3};
        control_mode.dso_q_setpoint =
            dt::DSOQSetpoint{dt::RationalNumber{7, 2}, std::nullopt, std::nullopt, false, dt::RationalNumber{0, 0}};
        control_mode.dso_cos_phi_setpoint = dt::DSOCosPhiSetpoint{
            dt::RationalNumber{95, -2}, std::nullopt, std::nullopt, dt::DERPowerFactorExcitation::UnderExcited, false,
            dt::RationalNumber{0, 0}};

        iso20_ac_AC_ChargeLoopResType out;
        message_20::convert(res, out);

        THEN("The generated cbv2g structure selects the dynamic DER branch") {
            REQUIRE(out.DER_Dynamic_AC_CLResControlMode_isUsed);
            const auto& der = out.DER_Dynamic_AC_CLResControlMode;
            REQUIRE(der.EVSETargetActivePower_L3_isUsed);
            REQUIRE(same_rational(der.EVSETargetActivePower_L3, dt::RationalNumber{3, 0}));
            REQUIRE(der.DSOQSetpoint_isUsed);
            REQUIRE(der.DSOCosPhiSetpoint_isUsed);
        }
    }
}

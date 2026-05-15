// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Vedecom Contributors to EVerest
#include <catch2/catch_test_macros.hpp>

#include "helper.hpp"

#include <iso15118/d20/control_event.hpp>
#include <iso15118/d20/state/ac_charge_parameter_discovery.hpp>
#include <iso15118/d20/state/authorization.hpp>
#include <iso15118/d20/state/authorization_setup.hpp>
#include <iso15118/d20/state/schedule_exchange.hpp>
#include <iso15118/d20/state/service_detail.hpp>
#include <iso15118/d20/state/service_discovery.hpp>
#include <iso15118/d20/state/service_selection.hpp>
#include <iso15118/d20/state/session_setup.hpp>
#include <iso15118/d20/state/supported_app_protocol.hpp>

#include <iso15118/message/ac_charge_parameter_discovery.hpp>
#include <iso15118/message/authorization.hpp>
#include <iso15118/message/authorization_setup.hpp>
#include <iso15118/message/service_detail.hpp>
#include <iso15118/message/service_discovery.hpp>
#include <iso15118/message/service_selection.hpp>
#include <iso15118/message/session_setup.hpp>
#include <iso15118/message/supported_app_protocol.hpp>

using namespace iso15118;

namespace {

namespace dt = message_20::datatypes;

d20::AcTransferLimits make_ac_limits() {
    d20::AcTransferLimits limits;
    limits.charge_power = {{22, 3}, {1, 3}};
    limits.nominal_frequency = {50, 0};
    limits.discharge_power = d20::Limit<dt::RationalNumber>{{11, 3}, {0, 0}};
    return limits;
}

message_20::Header header(const d20::Session& session) {
    return {session.get_id(), 1691411798};
}

} // namespace

SCENARIO("AC_DER_IEC Dynamic EIM FSM integration path") {
    const auto evse_id = std::string("ac der iec secc");
    const std::vector<dt::ServiceCategory> supported_energy_services = {dt::ServiceCategory::AC_DER};
    const std::vector<dt::Authorization> auth_services = {dt::Authorization::EIM};
    const std::vector<uint16_t> vas_services{};
    const d20::DcTransferLimits dc_limits;
    const d20::DcTransferLimits powersupply_limits;
    const auto cert_install{false};
    const std::vector<d20::ControlMobilityNeedsModes> control_mobility_modes = {
        {dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc}};
    const d20::EvseSetupConfig evse_setup{
        evse_id,          supported_energy_services, auth_services, vas_services, cert_install, dc_limits,
        make_ac_limits(), control_mobility_modes,    std::nullopt,  std::nullopt, std::nullopt, powersupply_limits};

    std::optional<d20::PauseContext> pause_ctx{std::nullopt};
    const session::feedback::Callbacks callbacks{};
    auto state_helper = FsmStateHelper(d20::SessionConfig(evse_setup), pause_ctx, callbacks);
    auto& ctx = state_helper.get_context();
    fsm::v2::FSM<d20::StateBase> fsm{ctx.create_state<d20::state::SupportedAppProtocol>()};

    GIVEN("An EV negotiates ISO 15118-20 AC and selects AC_DER with EIM") {
        message_20::SupportedAppProtocolRequest supported_app_req;
        auto& app_protocol = supported_app_req.app_protocol.emplace_back();
        app_protocol.priority = 1;
        app_protocol.protocol_namespace = "urn:iso:std:iso:15118:-20:AC";
        app_protocol.schema_id = 1;
        app_protocol.version_number_major = 1;
        app_protocol.version_number_minor = 0;

        state_helper.handle_request(supported_app_req);
        auto result = fsm.feed(d20::Event::V2GTP_MESSAGE);

        REQUIRE(result.transitioned());
        REQUIRE(fsm.get_current_state_id() == d20::StateID::SessionSetup);
        auto supported_app_res = ctx.get_response<message_20::SupportedAppProtocolResponse>();
        REQUIRE(supported_app_res.has_value());
        REQUIRE(supported_app_res->response_code ==
                message_20::SupportedAppProtocolResponse::ResponseCode::OK_SuccessfulNegotiation);

        const auto session_setup_req = message_20::SessionSetupRequest{
            message_20::Header{{0, 0, 0, 0, 0, 0, 0, 0}, 1691411798}, "WMIV1234567890ABCDEX"};
        state_helper.handle_request(session_setup_req);
        result = fsm.feed(d20::Event::V2GTP_MESSAGE);

        REQUIRE(result.transitioned());
        REQUIRE(fsm.get_current_state_id() == d20::StateID::AuthorizationSetup);
        REQUIRE(ctx.session.get_id() != dt::SessionId{0});

        state_helper.handle_request(message_20::AuthorizationSetupRequest{header(ctx.session)});
        result = fsm.feed(d20::Event::V2GTP_MESSAGE);

        REQUIRE(result.transitioned());
        REQUIRE(fsm.get_current_state_id() == d20::StateID::Authorization);
        auto auth_setup_res = ctx.get_response<message_20::AuthorizationSetupResponse>();
        REQUIRE(auth_setup_res.has_value());
        REQUIRE(auth_setup_res->response_code == dt::ResponseCode::OK);
        REQUIRE(auth_setup_res->authorization_services.size() == 1);
        REQUIRE(auth_setup_res->authorization_services[0] == dt::Authorization::EIM);

        state_helper.handle_control_event(d20::AuthorizationResponse(true));
        result = fsm.feed(d20::Event::CONTROL_MESSAGE);
        REQUIRE_FALSE(result.transitioned());

        message_20::AuthorizationRequest auth_req;
        auth_req.header = header(ctx.session);
        auth_req.selected_authorization_service = dt::Authorization::EIM;
        auth_req.authorization_mode.emplace<dt::EIM_ASReqAuthorizationMode>();
        state_helper.handle_request(auth_req);
        result = fsm.feed(d20::Event::V2GTP_MESSAGE);

        REQUIRE(result.transitioned());
        REQUIRE(fsm.get_current_state_id() == d20::StateID::ServiceDiscovery);
        auto auth_res = ctx.get_response<message_20::AuthorizationResponse>();
        REQUIRE(auth_res.has_value());
        REQUIRE(auth_res->response_code == dt::ResponseCode::OK);
        REQUIRE(auth_res->evse_processing == dt::Processing::Finished);

        message_20::ServiceDiscoveryRequest service_discovery_req;
        service_discovery_req.header = header(ctx.session);
        service_discovery_req.supported_service_ids = {message_20::to_underlying_value(dt::ServiceCategory::AC_DER)};
        state_helper.handle_request(service_discovery_req);
        result = fsm.feed(d20::Event::V2GTP_MESSAGE);

        REQUIRE(result.transitioned());
        REQUIRE(fsm.get_current_state_id() == d20::StateID::ServiceDetail);
        auto service_discovery_res = ctx.get_response<message_20::ServiceDiscoveryResponse>();
        REQUIRE(service_discovery_res.has_value());
        REQUIRE(service_discovery_res->response_code == dt::ResponseCode::OK);
        REQUIRE(service_discovery_res->energy_transfer_service_list.size() == 1);
        REQUIRE(service_discovery_res->energy_transfer_service_list[0].service_id == dt::ServiceCategory::AC_DER);

        const auto service_detail_req = message_20::ServiceDetailRequest{
            header(ctx.session), message_20::to_underlying_value(dt::ServiceCategory::AC_DER)};
        state_helper.handle_request(service_detail_req);
        result = fsm.feed(d20::Event::V2GTP_MESSAGE);

        REQUIRE(result.transitioned());
        REQUIRE(fsm.get_current_state_id() == d20::StateID::ServiceSelection);
        auto service_detail_res = ctx.get_response<message_20::ServiceDetailResponse>();
        REQUIRE(service_detail_res.has_value());
        REQUIRE(service_detail_res->response_code == dt::ResponseCode::OK);
        REQUIRE(service_detail_res->service == message_20::to_underlying_value(dt::ServiceCategory::AC_DER));
        REQUIRE_FALSE(service_detail_res->service_parameter_list.empty());

        message_20::ServiceSelectionRequest service_selection_req;
        service_selection_req.header = header(ctx.session);
        service_selection_req.selected_energy_transfer_service = {dt::ServiceCategory::AC_DER, 0};
        state_helper.handle_request(service_selection_req);
        result = fsm.feed(d20::Event::V2GTP_MESSAGE);

        REQUIRE(result.transitioned());
        REQUIRE(fsm.get_current_state_id() == d20::StateID::AC_ChargeParameterDiscovery);
        auto selected_services = ctx.session.get_selected_services();
        REQUIRE(selected_services.selected_energy_service == dt::ServiceCategory::AC_DER);
        REQUIRE(selected_services.selected_control_mode == dt::ControlMode::Dynamic);
        REQUIRE(selected_services.selected_der_control_functions.has_value());
        REQUIRE(selected_services.selected_der_control_functions->volt_watt);
        REQUIRE(selected_services.selected_der_control_functions->dso_q_setpoint_provision);
        REQUIRE(selected_services.selected_der_control_functions->dso_cos_phi_setpoint_provision);

        message_20::AC_ChargeParameterDiscoveryRequest cpd_req;
        cpd_req.header = header(ctx.session);
        auto& der_cpd = cpd_req.transfer_mode.emplace<dt::DER_AC_CPDReqEnergyTransferMode>();
        der_cpd.processing = dt::Processing::Finished;
        der_cpd.max_charge_power = {22, 3};
        der_cpd.min_charge_power = {1, 3};
        der_cpd.max_discharge_power = {11, 3};
        der_cpd.min_discharge_power = {0, 0};
        state_helper.handle_request(cpd_req);
        result = fsm.feed(d20::Event::V2GTP_MESSAGE);

        REQUIRE(result.transitioned());
        REQUIRE(fsm.get_current_state_id() == d20::StateID::ScheduleExchange);
        auto cpd_res = ctx.get_response<message_20::AC_ChargeParameterDiscoveryResponse>();
        REQUIRE(cpd_res.has_value());
        REQUIRE(cpd_res->response_code == dt::ResponseCode::OK);
        REQUIRE(std::holds_alternative<dt::DER_AC_CPDResEnergyTransferMode>(cpd_res->transfer_mode));
        const auto& der_cpd_res = std::get<dt::DER_AC_CPDResEnergyTransferMode>(cpd_res->transfer_mode);
        REQUIRE(der_cpd_res.der_control.active_power_support.has_value());
        REQUIRE(der_cpd_res.der_control.active_power_support->volt_watt.has_value());
        REQUIRE(der_cpd_res.der_control.reactive_power_support.has_value());
        REQUIRE(der_cpd_res.der_control.reactive_power_support->volt_var.has_value());
        REQUIRE(der_cpd_res.der_control.maximum_level_dc_injection.has_value());
    }
}

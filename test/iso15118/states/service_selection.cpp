// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest
#include <catch2/catch_test_macros.hpp>

#include <iso15118/detail/d20/state/service_selection.hpp>

using namespace iso15118;

namespace dt = message_20::datatypes;

SCENARIO("Service selection state handling") {
    GIVEN("Bad case - Unknown session") {

        d20::Session session = d20::Session();

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::DC_BPT;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        session = d20::Session();

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: FAILED_UnknownSession, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED_UnknownSession);
        }
    }

    GIVEN("Bad case: selected_energy_transfer_service false parameter set id - FAILED_ServiceSelectionInvalid") {

        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::DC};
        session.offered_services.dc_parameter_list[0] = {
            dt::DcConnector::Extended,
            dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc,
            dt::Pricing::NoPricing,
        };

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::DC;
        req.selected_energy_transfer_service.parameter_set_id = 1;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: FAILED_ServiceSelectionInvalid, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED_ServiceSelectionInvalid);
        }
    }

    GIVEN("Bad case: selected_energy_transfer service is not correct - FAILED_NoEnergyTransferServiceSelected") {

        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::DC};
        session.offered_services.dc_parameter_list[0] = {
            dt::DcConnector::Extended,
            dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc,
            dt::Pricing::NoPricing,
        };

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::AC;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: FAILED_NoEnergyTransferServiceSelected, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED_NoEnergyTransferServiceSelected);
        }
    }

    GIVEN("Good case") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::DC};
        session.offered_services.dc_parameter_list[0] = {
            dt::DcConnector::Extended,
            dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc,
            dt::Pricing::NoPricing,
        };

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::DC;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: OK") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
        }
    }

    GIVEN("Good case - Check if session variables is set") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::DC};
        session.offered_services.dc_parameter_list[0] = {
            dt::DcConnector::Extended,
            dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc,
            dt::Pricing::NoPricing,
        };

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::DC;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: OK") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);

            const auto selected_services = session.get_selected_services();
            REQUIRE(selected_services.selected_energy_service == dt::ServiceCategory::DC);
            REQUIRE(selected_services.selected_control_mode == dt::ControlMode::Scheduled);
        }
    }

    GIVEN("Good case - DC_BPT") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::DC_BPT};
        session.offered_services.dc_bpt_parameter_list[0] = {{
                                                                 dt::DcConnector::Extended,
                                                                 dt::ControlMode::Scheduled,
                                                                 dt::MobilityNeedsMode::ProvidedByEvcc,
                                                                 dt::Pricing::NoPricing,
                                                             },
                                                             dt::BptChannel::Unified,
                                                             dt::GeneratorMode::GridFollowing};

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::DC_BPT;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: OK") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);

            const auto selected_services = session.get_selected_services();
            REQUIRE(selected_services.selected_energy_service == dt::ServiceCategory::DC_BPT);
            REQUIRE(selected_services.selected_control_mode == dt::ControlMode::Scheduled);
        }
    }

    GIVEN("Good case - AC_DER") {
        d20::Session session = d20::Session();

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
        der_control_functions.standard_bitmap = 0x3f;
        der_control_functions.extended_bitmap = 0x3f;

        session.offered_services.energy_services = {dt::ServiceCategory::AC_DER};
        session.offered_services.ac_der_parameter_list[0] = {
            {{dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc, 230,
              dt::Pricing::NoPricing},
             dt::BptChannel::Unified,
             dt::GeneratorMode::GridFollowing,
             dt::GridCodeIslandingDetectionMethod::Passive},
            der_control_functions};

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::AC_DER;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: OK and DER parameters are selected") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);

            const auto selected_services = session.get_selected_services();
            REQUIRE(selected_services.selected_energy_service == dt::ServiceCategory::AC_DER);
            REQUIRE(selected_services.selected_control_mode == dt::ControlMode::Dynamic);
            REQUIRE(selected_services.selected_der_control_functions.has_value());
            REQUIRE(selected_services.selected_der_control_functions->volt_watt);
            REQUIRE(selected_services.selected_der_control_functions->dso_q_setpoint_provision);
            REQUIRE(selected_services.selected_der_control_functions->dso_cos_phi_setpoint_provision);
            REQUIRE(selected_services.selected_der_control_functions->dc_injection_restriction);
            REQUIRE(selected_services.selected_der_control_functions->under_frequency_watt);
            REQUIRE(selected_services.selected_der_control_functions->over_frequency_watt);
            REQUIRE(selected_services.selected_der_control_functions->volt_var);
            REQUIRE(selected_services.selected_der_control_functions->watt_var);
            REQUIRE(selected_services.selected_der_control_functions->watt_cos_phi);
            REQUIRE(selected_services.selected_der_control_functions->over_voltage_fault_ride_through);
            REQUIRE(selected_services.selected_der_control_functions->under_voltage_fault_ride_through);
            REQUIRE(selected_services.selected_der_control_functions->zero_current);
            REQUIRE(selected_services.selected_der_control_functions->standard_bitmap == 0x3f);
            REQUIRE(selected_services.selected_der_control_functions->extended_bitmap == 0x3f);
        }
    }

    GIVEN("Bad case - AC_DER without under-voltage fault ride-through") {
        d20::Session session = d20::Session();

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
        der_control_functions.zero_current = true;
        der_control_functions.standard_bitmap = 0x3f;
        der_control_functions.extended_bitmap = 0x3f;

        session.offered_services.energy_services = {dt::ServiceCategory::AC_DER};
        session.offered_services.ac_der_parameter_list[0] = {
            {{dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc, 230,
              dt::Pricing::NoPricing},
             dt::BptChannel::Unified,
             dt::GeneratorMode::GridFollowing,
             dt::GridCodeIslandingDetectionMethod::Passive},
            der_control_functions};

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::AC_DER;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: FAILED_ServiceSelectionInvalid and AC_DER is not selected") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED_ServiceSelectionInvalid);

            const auto selected_services = session.get_selected_services();
            REQUIRE(selected_services.selected_energy_service != dt::ServiceCategory::AC_DER);
            REQUIRE_FALSE(selected_services.selected_der_control_functions.has_value());
        }
    }

    GIVEN("Bad case - AC_DER with incomplete DER control functions") {
        d20::Session session = d20::Session();

        dt::DERControlFunctions der_control_functions;
        der_control_functions.volt_watt = true;

        session.offered_services.energy_services = {dt::ServiceCategory::AC_DER};
        session.offered_services.ac_der_parameter_list[0] = {
            {{dt::AcConnector::ThreePhase, dt::ControlMode::Dynamic, dt::MobilityNeedsMode::ProvidedByEvcc, 230,
              dt::Pricing::NoPricing},
             dt::BptChannel::Unified,
             dt::GeneratorMode::GridFollowing,
             dt::GridCodeIslandingDetectionMethod::Passive},
            der_control_functions};

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::AC_DER;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: FAILED_ServiceSelectionInvalid and AC_DER is not selected") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED_ServiceSelectionInvalid);

            const auto selected_services = session.get_selected_services();
            REQUIRE(selected_services.selected_energy_service != dt::ServiceCategory::AC_DER);
            REQUIRE_FALSE(selected_services.selected_der_control_functions.has_value());
        }
    }

    GIVEN("Bad case: selected_vas_list false service id - FAILED_ServiceSelectionInvalid") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::DC};
        session.offered_services.dc_parameter_list[0] = {
            dt::DcConnector::Extended,
            dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc,
            dt::Pricing::NoPricing,
        };

        session.offered_services.vas_services = {message_20::to_underlying_value(dt::ServiceCategory::Internet)};
        session.offered_services.internet_parameter_list[0] = {
            dt::Protocol::Http,
            dt::Port::Port80,
        };

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::DC;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        req.selected_vas_list = {{message_20::to_underlying_value(dt::ServiceCategory::ParkingStatus), 0}};

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: FAILED_ServiceSelectionInvalid, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED_ServiceSelectionInvalid);
        }
    }

    GIVEN("Bad case: selected_vas_list false parameter set id - FAILED_ServiceSelectionInvalid") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::DC};
        session.offered_services.dc_parameter_list[0] = {
            dt::DcConnector::Extended,
            dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc,
            dt::Pricing::NoPricing,
        };

        session.offered_services.vas_services = {message_20::to_underlying_value(dt::ServiceCategory::Internet)};
        session.offered_services.internet_parameter_list[0] = {
            dt::Protocol::Http,
            dt::Port::Port80,
        };

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::DC;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        req.selected_vas_list = {{message_20::to_underlying_value(dt::ServiceCategory::Internet), 1}};

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: FAILED_ServiceSelectionInvalid, mandatory fields should be set") {
            REQUIRE(res.response_code == dt::ResponseCode::FAILED_ServiceSelectionInvalid);
        }
    }

    GIVEN("Good case - DC & Internet & Parking") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::DC};
        session.offered_services.dc_parameter_list[0] = {
            dt::DcConnector::Extended,
            dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc,
            dt::Pricing::NoPricing,
        };

        session.offered_services.vas_services = {message_20::to_underlying_value(dt::ServiceCategory::Internet),
                                                 message_20::to_underlying_value(dt::ServiceCategory::ParkingStatus)};
        session.offered_services.internet_parameter_list[0] = {
            dt::Protocol::Http,
            dt::Port::Port80,
        };

        session.offered_services.parking_parameter_list[0] = {
            dt::IntendedService::VehicleCheckIn,
            dt::ParkingStatus::ManualExternal,
        };

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::DC;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        req.selected_vas_list = {{message_20::to_underlying_value(dt::ServiceCategory::Internet), 0},
                                 {message_20::to_underlying_value(dt::ServiceCategory::ParkingStatus), 0}};

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: OK") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
        }
    }

    GIVEN("Good case - AC") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::AC};
        session.offered_services.ac_parameter_list[0] = {
            dt::AcConnector::ThreePhase, dt::ControlMode::Scheduled, dt::MobilityNeedsMode::ProvidedByEvcc, 230,
            dt::Pricing::NoPricing,
        };

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::AC;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: OK") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
        }
    }

    GIVEN("Good case - AC_BPT") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::AC_BPT};
        session.offered_services.ac_bpt_parameter_list[0] = {{
                                                                 dt::AcConnector::ThreePhase,
                                                                 dt::ControlMode::Scheduled,
                                                                 dt::MobilityNeedsMode::ProvidedByEvcc,
                                                                 230,
                                                                 dt::Pricing::NoPricing,
                                                             },
                                                             dt::BptChannel::Unified,
                                                             dt::GeneratorMode::GridFollowing,
                                                             dt::GridCodeIslandingDetectionMethod::Passive};

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::AC_BPT;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: OK") {
            const auto selected_services = session.get_selected_services();

            REQUIRE(res.response_code == dt::ResponseCode::OK);
            REQUIRE(selected_services.selected_energy_service == dt::ServiceCategory::AC_BPT);
            REQUIRE(selected_services.selected_control_mode == dt::ControlMode::Scheduled);
        }
    }

    GIVEN("Good case - MCS") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::MCS};
        session.offered_services.mcs_parameter_list[0] = {
            dt::McsConnector::Mcs,
            dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc,
            dt::Pricing::NoPricing,
        };

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::MCS;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: OK") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);

            const auto selected_services = session.get_selected_services();
            REQUIRE(selected_services.selected_energy_service == dt::ServiceCategory::MCS);
            REQUIRE(selected_services.selected_control_mode == dt::ControlMode::Scheduled);
            REQUIRE(*std::get_if<dt::McsConnector>(&selected_services.selected_connector) == dt::McsConnector::Mcs);
        }
    }

    GIVEN("Good case - MCS_BPT") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::MCS_BPT};
        session.offered_services.mcs_bpt_parameter_list[0] = {{
                                                                  dt::McsConnector::Mcs,
                                                                  dt::ControlMode::Scheduled,
                                                                  dt::MobilityNeedsMode::ProvidedByEvcc,
                                                                  dt::Pricing::NoPricing,
                                                              },
                                                              dt::BptChannel::Unified,
                                                              dt::GeneratorMode::GridFollowing};

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::MCS_BPT;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: OK") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);

            const auto selected_services = session.get_selected_services();
            REQUIRE(selected_services.selected_energy_service == dt::ServiceCategory::MCS_BPT);
            REQUIRE(selected_services.selected_control_mode == dt::ControlMode::Scheduled);
            REQUIRE(*std::get_if<dt::McsConnector>(&selected_services.selected_connector) == dt::McsConnector::Mcs);
            const auto bpt_channel = selected_services.selected_bpt_channel.has_value() and
                                     selected_services.selected_bpt_channel.value() == dt::BptChannel::Unified;
            REQUIRE(bpt_channel == true);
        }
    }

    GIVEN("Good case - DC & Custom VAS") {
        d20::Session session = d20::Session();

        session.offered_services.energy_services = {dt::ServiceCategory::DC};
        session.offered_services.dc_parameter_list[0] = {
            dt::DcConnector::Extended,
            dt::ControlMode::Scheduled,
            dt::MobilityNeedsMode::ProvidedByEvcc,
            dt::Pricing::NoPricing,
        };

        session.offered_services.vas_services = {4599};
        session.offered_services.custom_vas_list[4599] = {0, 2};

        message_20::ServiceSelectionRequest req;
        req.header.session_id = session.get_id();
        req.header.timestamp = 1691411798;
        req.selected_energy_transfer_service.service_id = dt::ServiceCategory::DC;
        req.selected_energy_transfer_service.parameter_set_id = 0;

        req.selected_vas_list = {
            {4599, 0},
        };

        const auto res = d20::state::handle_request(req, session);

        THEN("ResponseCode: OK") {
            REQUIRE(res.response_code == dt::ResponseCode::OK);
        }
    }

    // GIVEN("Bad case - FAILED_NoServiceRenegotiationSupported") {} // todo(sl): pause/resume not supported yet

    // GIVEN("Bad Case - sequence error") {} // TODO(sl): not here

    // GIVEN("Bad Case - Performance Timeout") {} // TODO(sl): not here

    // GIVEN("Bad Case - Sequence Timeout") {} // TODO(sl): not here
}

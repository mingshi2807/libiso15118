#include <catch2/catch_test_macros.hpp>

#include <iso15118/message/ac_charge_parameter_discovery.hpp>
#include <iso15118/message/variant.hpp>

#include "helper.hpp"

using namespace iso15118;

namespace {

namespace dt = iso15118::message_20::datatypes;

dt::RationalNumber rn(const int16_t value, const int8_t exponent = 0) {
    return {value, exponent};
}

void require_rn(const dt::RationalNumber& actual, const dt::RationalNumber& expected) {
    REQUIRE(actual.value == expected.value);
    REQUIRE(actual.exponent == expected.exponent);
}

dt::DERCurve der_curve(const dt::DERCurveDataUnit x_unit, const dt::DERCurveDataUnit y_unit) {
    dt::DERCurve curve;
    curve.x_unit = x_unit;
    curve.y_unit = y_unit;
    curve.curve_data_points = {
        {rn(230), {rn(0), std::nullopt}},
        {rn(240), {rn(70), dt::DERPowerFactorExcitation::UnderExcited}},
    };
    curve.min_cos_phi = rn(90, -2);
    curve.lock_value_unit = x_unit;
    curve.lock_in_value = rn(5);
    curve.lock_out_value = rn(10);
    curve.pt1_response_reactive_power = true;
    curve.step_response_time_constant_reactive_power = rn(2);
    curve.intentional_delay = rn(1);
    return curve;
}

dt::FrequencyWatt frequency_watt(const dt::DERPowerReference power_reference) {
    dt::FrequencyWatt value;
    value.f_start = rn(498, -1);
    value.f_stop = rn(495, -1);
    value.intentional_delay_f_stop = 2;
    value.slope = rn(40);
    value.deactivation_time = 4;
    value.intentional_delay_power_control = 3;
    value.power_reference = power_reference;
    value.hysteresis_control = true;
    value.power_up_ramp = 6;
    value.pt1_response_active_power = true;
    value.step_response_time_constant_active_power = rn(1);
    return value;
}

dt::FaultRideThrough fault_ride_through() {
    dt::FaultRideThrough value;
    value.voltage_limit_start_frt = rn(253);
    value.voltage_limit_stop_frt = rn(248);
    value.voltage_recovery_limit = rn(245);
    value.voltage_ride_through_positive_curve_k_factor = rn(2);
    value.voltage_ride_through_negative_curve_k_factor = rn(-2);
    value.pt1_response_active_power = true;
    value.step_response_time_constant_active_power = rn(1);
    value.pt1_response_reactive_power = true;
    value.step_response_time_constant_reactive_power = rn(2);
    return value;
}

dt::ZeroCurrent zero_current() {
    dt::ZeroCurrent value;
    value.over_voltage_limit = rn(260);
    value.under_voltage_limit = rn(180);
    value.over_voltage_recovery_limit = rn(250);
    value.under_voltage_recovery_limit = rn(190);
    value.pt1_response_active_power = true;
    value.step_response_time_constant_active_power = rn(1);
    value.pt1_response_reactive_power = true;
    value.step_response_time_constant_reactive_power = rn(2);
    return value;
}

void require_der_curve(const dt::DERCurve& curve, const dt::DERCurveDataUnit x_unit, const dt::DERCurveDataUnit y_unit) {
    REQUIRE(curve.x_unit == x_unit);
    REQUIRE(curve.y_unit == y_unit);
    REQUIRE(curve.curve_data_points.size() == 2);
    require_rn(curve.curve_data_points[0].x_value, rn(230));
    require_rn(curve.curve_data_points[0].y_value.setpoint_value, rn(0));
    REQUIRE_FALSE(curve.curve_data_points[0].y_value.excitation.has_value());
    require_rn(curve.curve_data_points[1].x_value, rn(240));
    require_rn(curve.curve_data_points[1].y_value.setpoint_value, rn(70));
    REQUIRE(curve.curve_data_points[1].y_value.excitation == dt::DERPowerFactorExcitation::UnderExcited);
    REQUIRE(curve.lock_value_unit == x_unit);
    require_rn(*curve.lock_in_value, rn(5));
    require_rn(*curve.lock_out_value, rn(10));
}

} // namespace

SCENARIO("Se/Deserialize ac charge parameter discovery messages") {

    GIVEN("Deserialize ac_charge_parameter_discovery_req") {

        uint8_t doc_raw[] = {0x80, 0x10, 0x04, 0x1e, 0xa6, 0x5f, 0xc9, 0x9b, 0xa7, 0x6c, 0x4d, 0x8c, 0x3b,
                             0xfe, 0x1b, 0x60, 0x62, 0x07, 0xE0, 0x80, 0x19, 0x02, 0x00, 0x00, 0x00, 0x80,
                             0x00, 0x00, 0x3F, 0x06, 0x80, 0x78, 0x10, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00};

        const io::StreamInputView stream_view{doc_raw, sizeof(doc_raw)};

        message_20::Variant variant(io::v2gtp::PayloadType::Part20AC, stream_view);

        THEN("It should be deserialized successfully") {
            REQUIRE(variant.get_type() == message_20::Type::AC_ChargeParameterDiscoveryReq);

            const auto& msg = variant.get<message_20::AC_ChargeParameterDiscoveryRequest>();
            const auto& header = msg.header;

            REQUIRE(header.session_id == std::array<uint8_t, 8>{0x3D, 0x4C, 0xBF, 0x93, 0x37, 0x4E, 0xD8, 0x9B});
            REQUIRE(header.timestamp == 1725456323);

            using AC_ModeReq = message_20::datatypes::AC_CPDReqEnergyTransferMode;

            REQUIRE(std::holds_alternative<AC_ModeReq>(msg.transfer_mode));
            const auto& transfer_mode = std::get<AC_ModeReq>(msg.transfer_mode);
            REQUIRE(message_20::datatypes::from_RationalNumber(transfer_mode.max_charge_power) == 32);
            REQUIRE(transfer_mode.max_charge_power_L2.has_value() == true);
            REQUIRE(message_20::datatypes::from_RationalNumber(*transfer_mode.max_charge_power_L2) == 0.0f);
            REQUIRE(transfer_mode.max_charge_power_L3.has_value() == true);
            REQUIRE(message_20::datatypes::from_RationalNumber(*transfer_mode.max_charge_power_L3) == 0.0f);
            REQUIRE(message_20::datatypes::from_RationalNumber(transfer_mode.min_charge_power) == 20);
            REQUIRE(transfer_mode.min_charge_power_L2.has_value() == true);
            REQUIRE(message_20::datatypes::from_RationalNumber(*transfer_mode.min_charge_power_L2) == 0.0f);
            REQUIRE(transfer_mode.min_charge_power_L3.has_value() == true);
            REQUIRE(message_20::datatypes::from_RationalNumber(*transfer_mode.min_charge_power_L3) == 0.0f);
        }
    }

    GIVEN("Serialize ac_charge_parameter_discovery_req") {

        using AC_ModeReq = message_20::datatypes::AC_CPDReqEnergyTransferMode;

        message_20::AC_ChargeParameterDiscoveryRequest req;

        req.header = message_20::Header{{0x3D, 0x4C, 0xBF, 0x93, 0x37, 0x4E, 0xD8, 0x9B}, 1725456323};
        auto& mode = req.transfer_mode.emplace<AC_ModeReq>();
        mode.max_charge_power = {3200, -2};
        mode.max_charge_power_L2 = {0, 0};
        mode.max_charge_power_L3 = {0, 0};
        mode.min_charge_power = {2000, -2};
        mode.min_charge_power_L2 = {0, 0};
        mode.min_charge_power_L3 = {0, 0};

        std::vector<uint8_t> expected = {0x80, 0x10, 0x04, 0x1e, 0xa6, 0x5f, 0xc9, 0x9b, 0xa7, 0x6c, 0x4d, 0x8c, 0x3b,
                                         0xfe, 0x1b, 0x60, 0x62, 0x07, 0xE0, 0x80, 0x19, 0x02, 0x00, 0x00, 0x00, 0x80,
                                         0x00, 0x00, 0x3F, 0x06, 0x80, 0x78, 0x10, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00};

        THEN("It should be serialized successfully") {
            REQUIRE(serialize_helper(req) == expected);
        }
    }

    // TODO(sl): Adding BPT_AC_CPDReqEnergyTransferMode tests
    // TODO(rb): Adding BPT_AC_CPDResEnergyTransferMode tests

    GIVEN("Deserialize ac_charge_parameter_discovery_res") {

        uint8_t doc_raw[] = {0x80, 0x14, 0x04, 0x1e, 0xa6, 0x5f, 0xc9, 0x9b, 0xa7, 0x6c, 0x4d, 0x8c, 0x4b, 0xfe, 0x1b,
                             0x60, 0x62, 0x00, 0x04, 0x08, 0x50, 0x08, 0x81, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x20,
                             0x43, 0x40, 0x3c, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x05, 0x00};

        const io::StreamInputView stream_view{doc_raw, sizeof(doc_raw)};

        message_20::Variant variant(io::v2gtp::PayloadType::Part20AC, stream_view);

        THEN("It should be deserialized successfully") {
            REQUIRE(variant.get_type() == message_20::Type::AC_ChargeParameterDiscoveryRes);

            const auto& msg = variant.get<message_20::AC_ChargeParameterDiscoveryResponse>();
            const auto& header = msg.header;

            REQUIRE(header.session_id == std::array<uint8_t, 8>{0x3D, 0x4C, 0xBF, 0x93, 0x37, 0x4E, 0xD8, 0x9B});
            REQUIRE(header.timestamp == 1725456324);
            REQUIRE(msg.response_code == message_20::datatypes::ResponseCode::OK);

            using AC_ModeRes = message_20::datatypes::AC_CPDResEnergyTransferMode;

            REQUIRE(std::holds_alternative<AC_ModeRes>(msg.transfer_mode));
            const auto& transfer_mode = std::get<AC_ModeRes>(msg.transfer_mode);
            REQUIRE(message_20::datatypes::from_RationalNumber(transfer_mode.max_charge_power) == 22080);
            REQUIRE(transfer_mode.max_charge_power_L2.has_value() == true);
            REQUIRE(message_20::datatypes::from_RationalNumber(*transfer_mode.max_charge_power_L2) == 0.0f);
            REQUIRE(transfer_mode.max_charge_power_L3.has_value() == true);
            REQUIRE(message_20::datatypes::from_RationalNumber(*transfer_mode.max_charge_power_L3) == 0.0f);
            REQUIRE(message_20::datatypes::from_RationalNumber(transfer_mode.min_charge_power) == 20000);
            REQUIRE(transfer_mode.min_charge_power_L2.has_value() == true);
            REQUIRE(message_20::datatypes::from_RationalNumber(*transfer_mode.min_charge_power_L2) == 0.0f);
            REQUIRE(transfer_mode.min_charge_power_L3.has_value() == true);
            REQUIRE(message_20::datatypes::from_RationalNumber(*transfer_mode.min_charge_power_L3) == 0.0f);
        }
    }

    GIVEN("Serialize ac_charge_parameter_discovery_res") {

        using AC_ModeRes = message_20::datatypes::AC_CPDResEnergyTransferMode;

        message_20::AC_ChargeParameterDiscoveryResponse res;

        res.header = message_20::Header{{0x3D, 0x4C, 0xBF, 0x93, 0x37, 0x4E, 0xD8, 0x9B}, 1725456324};
        res.response_code = message_20::datatypes::ResponseCode::OK;
        auto& mode = res.transfer_mode.emplace<AC_ModeRes>();
        mode.max_charge_power = {2208, 1};
        mode.max_charge_power_L2 = {0, 0};
        mode.max_charge_power_L3 = {0, 0};
        mode.min_charge_power = {2000, 1};
        mode.min_charge_power_L2 = {0, 0};
        mode.min_charge_power_L3 = {0, 0};
        std::vector<uint8_t> expected = {0x80, 0x14, 0x04, 0x1e, 0xa6, 0x5f, 0xc9, 0x9b, 0xa7, 0x6c, 0x4d,
                                         0x8c, 0x4b, 0xfe, 0x1b, 0x60, 0x62, 0x00, 0x04, 0x08, 0x50, 0x08,
                                         0x81, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x20, 0x43, 0x40, 0x3c,
                                         0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x05, 0x00};

        THEN("It should be serialized successfully") {
            REQUIRE(serialize_helper(res) == expected);
        }
    }

    // TODO(sl): Adding BPT_AC_CPDResEnergyTransferMode tests
    // TODO(rb): Adding BPT_AC_CPDReqEnergyTransferMode tests

    GIVEN("Round-trip DER ac_charge_parameter_discovery_req") {

        using DER_ModeReq = message_20::datatypes::DER_AC_CPDReqEnergyTransferMode;

        message_20::AC_ChargeParameterDiscoveryRequest req;

        req.header = message_20::Header{{0x3D, 0x4C, 0xBF, 0x93, 0x37, 0x4E, 0xD8, 0x9B}, 1725456400};
        auto& mode = req.transfer_mode.emplace<DER_ModeReq>();
        mode.max_charge_power = rn(3200, -2);
        mode.max_charge_power_L2 = rn(1600, -2);
        mode.max_charge_power_L3 = rn(1600, -2);
        mode.min_charge_power = rn(2000, -2);
        mode.min_charge_power_L2 = rn(1000, -2);
        mode.min_charge_power_L3 = rn(1000, -2);
        mode.max_discharge_power = rn(1100, -2);
        mode.max_discharge_power_L2 = rn(550, -2);
        mode.max_discharge_power_L3 = rn(550, -2);
        mode.min_discharge_power = rn(100, -2);
        mode.min_discharge_power_L2 = rn(50, -2);
        mode.min_discharge_power_L3 = rn(50, -2);
        mode.processing = dt::Processing::Ongoing;
        mode.session_total_discharge_energy_available = rn(15, 3);
        auto& reactive_limits = mode.reactive_power_limits.emplace();
        reactive_limits.max_charge_reactive_power = rn(700);
        reactive_limits.max_charge_reactive_power_L2 = rn(350);
        reactive_limits.max_charge_reactive_power_L3 = rn(350);
        reactive_limits.min_charge_reactive_power = rn(-700);
        reactive_limits.min_charge_reactive_power_L2 = rn(-350);
        reactive_limits.min_charge_reactive_power_L3 = rn(-350);
        reactive_limits.max_discharge_reactive_power = rn(600);
        reactive_limits.max_discharge_reactive_power_L2 = rn(300);
        reactive_limits.max_discharge_reactive_power_L3 = rn(300);
        reactive_limits.min_discharge_reactive_power = rn(-600);
        reactive_limits.min_discharge_reactive_power_L2 = rn(-300);
        reactive_limits.min_discharge_reactive_power_L3 = rn(-300);

        const auto serialized = serialize_helper(req);
        const io::StreamInputView stream_view{serialized.data(), serialized.size()};
        message_20::Variant variant(io::v2gtp::PayloadType::Part20AC, stream_view);

        THEN("DER AC CPD request fields should survive EXI round-trip") {
            REQUIRE(variant.get_type() == message_20::Type::AC_ChargeParameterDiscoveryReq);

            const auto& msg = variant.get<message_20::AC_ChargeParameterDiscoveryRequest>();
            REQUIRE(std::holds_alternative<DER_ModeReq>(msg.transfer_mode));

            const auto& transfer_mode = std::get<DER_ModeReq>(msg.transfer_mode);
            REQUIRE(transfer_mode.processing == dt::Processing::Ongoing);
            require_rn(transfer_mode.max_charge_power, rn(3200, -2));
            require_rn(transfer_mode.max_discharge_power, rn(1100, -2));
            require_rn(*transfer_mode.session_total_discharge_energy_available, rn(15, 3));
            REQUIRE(transfer_mode.reactive_power_limits.has_value());
            require_rn(transfer_mode.reactive_power_limits->max_charge_reactive_power, rn(700));
            require_rn(*transfer_mode.reactive_power_limits->min_discharge_reactive_power_L3, rn(-300));
        }
    }

    GIVEN("Round-trip DER ac_charge_parameter_discovery_res with AC DER control functions") {

        using DER_ModeRes = message_20::datatypes::DER_AC_CPDResEnergyTransferMode;

        message_20::AC_ChargeParameterDiscoveryResponse res;

        res.header = message_20::Header{{0x3D, 0x4C, 0xBF, 0x93, 0x37, 0x4E, 0xD8, 0x9B}, 1725456401};
        res.response_code = dt::ResponseCode::OK;
        auto& mode = res.transfer_mode.emplace<DER_ModeRes>();
        mode.max_charge_power = rn(2208, 1);
        mode.max_charge_power_L2 = rn(1104, 1);
        mode.max_charge_power_L3 = rn(1104, 1);
        mode.min_charge_power = rn(2000, 1);
        mode.min_charge_power_L2 = rn(1000, 1);
        mode.min_charge_power_L3 = rn(1000, 1);
        mode.nominal_frequency = rn(50);
        mode.max_power_asymmetry = rn(5);
        mode.power_ramp_limitation = rn(10);
        mode.present_active_power = rn(1200);
        mode.present_active_power_L2 = rn(600);
        mode.present_active_power_L3 = rn(600);
        mode.nominal_charge_power = rn(1104, 1);
        mode.nominal_charge_power_L2 = rn(552, 1);
        mode.nominal_charge_power_L3 = rn(552, 1);
        mode.nominal_discharge_power = rn(800, 1);
        mode.nominal_discharge_power_L2 = rn(400, 1);
        mode.nominal_discharge_power_L3 = rn(400, 1);
        mode.max_discharge_power = rn(900, 1);
        mode.max_discharge_power_L2 = rn(450, 1);
        mode.max_discharge_power_L3 = rn(450, 1);
        mode.operating_mode = dt::EVOperatingMode::GridFollowing;
        mode.grid_connection_mode = dt::GridConnectionMode::GridConnected;

        auto& der_control = mode.der_control;
        der_control.maximum_level_dc_injection = rn(5, -1);
        der_control.active_power_support.emplace();
        der_control.active_power_support->volt_watt =
            dt::VoltWatt{dt::DERPowerReference::MaximumDischargePower, rn(241), rn(253), true, rn(1), 3};
        der_control.active_power_support->under_frequency_watt =
            frequency_watt(dt::DERPowerReference::MaximumDischargePower);
        der_control.active_power_support->over_frequency_watt = frequency_watt(dt::DERPowerReference::MomentaryPower);
        der_control.reactive_power_support.emplace();
        der_control.reactive_power_support->volt_var = der_curve(dt::DERCurveDataUnit::V, dt::DERCurveDataUnit::var);
        der_control.reactive_power_support->watt_var = der_curve(dt::DERCurveDataUnit::W, dt::DERCurveDataUnit::var);
        der_control.reactive_power_support->watt_cos_phi =
            der_curve(dt::DERCurveDataUnit::W, dt::DERCurveDataUnit::var);
        der_control.overvoltage_fault_ride_through = fault_ride_through();
        der_control.undervoltage_fault_ride_through = fault_ride_through();
        der_control.zero_current = zero_current();

        const auto serialized = serialize_helper(res);
        const io::StreamInputView stream_view{serialized.data(), serialized.size()};
        message_20::Variant variant(io::v2gtp::PayloadType::Part20AC, stream_view);

        THEN("DER AC CPD response controls should survive EXI round-trip") {
            REQUIRE(variant.get_type() == message_20::Type::AC_ChargeParameterDiscoveryRes);

            const auto& msg = variant.get<message_20::AC_ChargeParameterDiscoveryResponse>();
            REQUIRE(msg.response_code == dt::ResponseCode::OK);
            REQUIRE(std::holds_alternative<DER_ModeRes>(msg.transfer_mode));

            const auto& transfer_mode = std::get<DER_ModeRes>(msg.transfer_mode);
            require_rn(transfer_mode.nominal_charge_power, rn(1104, 1));
            require_rn(transfer_mode.nominal_discharge_power, rn(800, 1));
            REQUIRE(transfer_mode.operating_mode == dt::EVOperatingMode::GridFollowing);
            REQUIRE(transfer_mode.grid_connection_mode == dt::GridConnectionMode::GridConnected);

            const auto& der_control = transfer_mode.der_control;
            require_rn(*der_control.maximum_level_dc_injection, rn(5, -1));
            REQUIRE(der_control.active_power_support.has_value());
            REQUIRE(der_control.active_power_support->volt_watt.has_value());
            require_rn(der_control.active_power_support->volt_watt->u_start, rn(241));
            require_rn(der_control.active_power_support->volt_watt->u_stop, rn(253));
            REQUIRE(der_control.active_power_support->under_frequency_watt.has_value());
            REQUIRE(der_control.active_power_support->over_frequency_watt.has_value());
            REQUIRE(der_control.reactive_power_support.has_value());
            REQUIRE(der_control.reactive_power_support->volt_var.has_value());
            REQUIRE(der_control.reactive_power_support->watt_var.has_value());
            REQUIRE(der_control.reactive_power_support->watt_cos_phi.has_value());
            require_der_curve(*der_control.reactive_power_support->volt_var, dt::DERCurveDataUnit::V,
                              dt::DERCurveDataUnit::var);
            require_der_curve(*der_control.reactive_power_support->watt_var, dt::DERCurveDataUnit::W,
                              dt::DERCurveDataUnit::var);
            require_der_curve(*der_control.reactive_power_support->watt_cos_phi, dt::DERCurveDataUnit::W,
                              dt::DERCurveDataUnit::var);
            REQUIRE(der_control.overvoltage_fault_ride_through.has_value());
            REQUIRE(der_control.undervoltage_fault_ride_through.has_value());
            REQUIRE(der_control.zero_current.has_value());
        }
    }
}

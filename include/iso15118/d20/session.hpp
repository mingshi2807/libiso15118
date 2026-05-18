// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include <iso15118/io/sha_hash.hpp>
#include <iso15118/message/common_types.hpp>

namespace iso15118::d20 {

namespace dt = message_20::datatypes;

using ServiceId = uint16_t;

// Key: service IDs, value: vector with the parameter set ids
using CustomVasList = std::map<std::uint16_t, std::vector<uint16_t>>;

struct OfferedServices {

    std::vector<dt::Authorization> auth_services;
    std::vector<dt::ServiceCategory> energy_services;
    std::vector<uint16_t> vas_services;

    std::map<uint8_t, dt::AcParameterList> ac_parameter_list;
    std::map<uint8_t, dt::AcBptParameterList> ac_bpt_parameter_list;
    std::map<uint8_t, dt::AcDerParameterList> ac_der_parameter_list;
    std::map<uint8_t, dt::DcParameterList> dc_parameter_list;
    std::map<uint8_t, dt::DcBptParameterList> dc_bpt_parameter_list;
    std::map<uint8_t, dt::McsParameterList> mcs_parameter_list;
    std::map<uint8_t, dt::McsBptParameterList> mcs_bpt_parameter_list;
    std::map<uint8_t, dt::InternetParameterList> internet_parameter_list;
    std::map<uint8_t, dt::ParkingParameterList> parking_parameter_list;
    CustomVasList custom_vas_list;
};

struct SelectedServiceParameters {

    dt::ServiceCategory selected_energy_service;

    std::variant<dt::AcConnector, dt::DcConnector, dt::McsConnector> selected_connector;
    dt::ControlMode selected_control_mode;
    dt::MobilityNeedsMode selected_mobility_needs_mode;
    dt::Pricing selected_pricing;

    // BPT
    std::optional<dt::BptChannel> selected_bpt_channel;
    std::optional<dt::GeneratorMode> selected_generator_mode;
    std::optional<dt::DERControlFunctions> selected_der_control_functions;

    // AC specific
    std::optional<float> evse_nominal_voltage;
    std::optional<dt::GridCodeIslandingDetectionMethod> selected_grid_code_method;

    using ConnectorVariant = std::variant<dt::AcConnector, dt::DcConnector, dt::McsConnector>;

    SelectedServiceParameters() = default;

    // Canonical constructor: mandatory fields + connector variant.
    SelectedServiceParameters(dt::ServiceCategory, ConnectorVariant, dt::ControlMode,
                              dt::MobilityNeedsMode, dt::Pricing);

    // Backward-compatible delegating constructors (prefer canonical + setters for new code).
    SelectedServiceParameters(dt::ServiceCategory s, dt::DcConnector c, dt::ControlMode m,
                              dt::MobilityNeedsMode n, dt::Pricing p)
        : SelectedServiceParameters(s, ConnectorVariant{c}, m, n, p) {}
    SelectedServiceParameters(dt::ServiceCategory s, dt::DcConnector c, dt::ControlMode m,
                              dt::MobilityNeedsMode n, dt::Pricing p, dt::BptChannel ch, dt::GeneratorMode g)
        : SelectedServiceParameters(s, ConnectorVariant{c}, m, n, p) {
        selected_bpt_channel = ch;
        selected_generator_mode = g;
    }
    SelectedServiceParameters(dt::ServiceCategory s, dt::McsConnector c, dt::ControlMode m,
                              dt::MobilityNeedsMode n, dt::Pricing p)
        : SelectedServiceParameters(s, ConnectorVariant{c}, m, n, p) {}
    SelectedServiceParameters(dt::ServiceCategory s, dt::McsConnector c, dt::ControlMode m,
                              dt::MobilityNeedsMode n, dt::Pricing p, dt::BptChannel ch, dt::GeneratorMode g)
        : SelectedServiceParameters(s, ConnectorVariant{c}, m, n, p) {
        selected_bpt_channel = ch;
        selected_generator_mode = g;
    }
    SelectedServiceParameters(dt::ServiceCategory s, dt::AcConnector c, dt::ControlMode m,
                              dt::MobilityNeedsMode n, dt::Pricing p, float nominal_voltage_)
        : SelectedServiceParameters(s, ConnectorVariant{c}, m, n, p) {
        evse_nominal_voltage = nominal_voltage_;
    }
    SelectedServiceParameters(dt::ServiceCategory s, dt::AcConnector c, dt::ControlMode m,
                              dt::MobilityNeedsMode n, dt::Pricing p, dt::BptChannel ch, dt::GeneratorMode g,
                              float v, dt::GridCodeIslandingDetectionMethod grid)
        : SelectedServiceParameters(s, ConnectorVariant{c}, m, n, p) {
        selected_bpt_channel = ch;
        selected_generator_mode = g;
        evse_nominal_voltage = v;
        selected_grid_code_method = grid;
    }
    SelectedServiceParameters(dt::ServiceCategory s, dt::AcConnector c, dt::ControlMode m,
                              dt::MobilityNeedsMode n, dt::Pricing p, dt::BptChannel ch, dt::GeneratorMode g,
                              float v, dt::GridCodeIslandingDetectionMethod grid, dt::DERControlFunctions der)
        : SelectedServiceParameters(s, ConnectorVariant{c}, m, n, p) {
        selected_bpt_channel = ch;
        selected_generator_mode = g;
        evse_nominal_voltage = v;
        selected_grid_code_method = grid;
        selected_der_control_functions = der;
    }

    // Fluent setters for optional fields (recommended pattern for new code).
    SelectedServiceParameters& with_bpt(dt::BptChannel ch, dt::GeneratorMode gen) {
        selected_bpt_channel = ch;
        selected_generator_mode = gen;
        return *this;
    }
    SelectedServiceParameters& with_voltage(float v) {
        evse_nominal_voltage = v;
        return *this;
    }
    SelectedServiceParameters& with_grid_code(dt::GridCodeIslandingDetectionMethod gc) {
        selected_grid_code_method = gc;
        return *this;
    }
    SelectedServiceParameters& with_der(dt::DERControlFunctions der) {
        selected_der_control_functions = der;
        return *this;
    }
};

// Todo(sl): missing services
// WPT -> ControlMode, Pricing
// DC_ACDP -> ControlMode, MobilityNeedsMode
// DC_ACDP_BPT -> ControlMode, MobilityNeedsMode, BPTChannel

struct SelectedVasParameter {
    std::vector<dt::ServiceCategory> vas_services;

    dt::Protocol internet_protocol;
    dt::Port internet_port;

    dt::IntendedService parking_intended_service;
    dt::ParkingStatus parking_status;
};

// TODO(SL): How to handle d2 pause? Move Struct to a seperate header file?
// TODO(SL): Missing handling scheduletuple in schedule mode [V2G20-1058]
struct PauseContext {
    io::sha512_hash_t vehicle_cert_session_id_hash{};
    std::array<uint8_t, 8> old_session_id{};
    SelectedServiceParameters selected_service_parameters{};
};

class Session {

    // TODO(sl): move to a common defs file
    static constexpr auto ID_LENGTH = 8;

public:
    Session();
    Session(const PauseContext& pause_ctx);
    Session(SelectedServiceParameters);
    Session(OfferedServices);

    std::array<uint8_t, ID_LENGTH> get_id() const {
        return id;
    }

    bool find_energy_parameter_set_id(const dt::ServiceCategory service, int16_t id);
    bool find_vas_parameter_set_id(const uint16_t vas_service, int16_t id);

    void selected_service_parameters(const dt::ServiceCategory service, const uint16_t id);
    void selected_service_parameters(const uint16_t vas_service, const uint16_t id);

    auto get_selected_services() const& {
        return selected_services;
    }

    bool is_ac_charger() const {
        return selected_services.selected_energy_service == dt::ServiceCategory::AC or
               selected_services.selected_energy_service == dt::ServiceCategory::AC_BPT or
               selected_services.selected_energy_service == dt::ServiceCategory::AC_DER;
    }

    bool is_dc_charger() const {
        return selected_services.selected_energy_service == dt::ServiceCategory::DC or
               selected_services.selected_energy_service == dt::ServiceCategory::DC_BPT or
               selected_services.selected_energy_service == dt::ServiceCategory::MCS or
               selected_services.selected_energy_service == dt::ServiceCategory::MCS_BPT;
    }

    ~Session();

    OfferedServices offered_services;

    bool service_renegotiation_supported{false};

private:
    static std::array<uint8_t, ID_LENGTH> generate_session_id();

    // NOTE (aw): could be const
    std::array<uint8_t, ID_LENGTH> id{};

    SelectedServiceParameters selected_services{};
    SelectedVasParameter selected_vas_services{};
};

} // namespace iso15118::d20
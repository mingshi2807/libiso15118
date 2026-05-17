// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Pionix GmbH and Contributors to EVerest
#pragma once

#include <iso15118/d20/ac_powers.hpp>
#include <iso15118/d20/config.hpp>
#include <iso15118/d20/session.hpp>
#include <iso15118/message/ac_charge_parameter_discovery.hpp>

namespace iso15118::d20::state {

struct AcChargeParameterDiscoveryResult {
    message_20::AC_ChargeParameterDiscoveryResponse response;
    d20::AcDerControlFailureReason ac_der_failure_reason{d20::AcDerControlFailureReason::None};
};

message_20::AC_ChargeParameterDiscoveryResponse
handle_request(const message_20::AC_ChargeParameterDiscoveryRequest& req, const d20::Session& session,
               const d20::AcTransferLimits& limits, const d20::AcPresentPower& powers);

message_20::AC_ChargeParameterDiscoveryResponse
handle_request(const message_20::AC_ChargeParameterDiscoveryRequest& req, const d20::Session& session,
               const d20::AcTransferLimits& limits, const d20::AcPresentPower& powers,
               const d20::AcDerControlConfig& ac_der_control_config);

message_20::AC_ChargeParameterDiscoveryResponse
handle_request(const message_20::AC_ChargeParameterDiscoveryRequest& req, const d20::Session& session,
               const d20::AcTransferLimits& limits, const d20::AcPresentPower& powers,
               const d20::IAcDerControlProvider& ac_der_control_provider);

AcChargeParameterDiscoveryResult
handle_request_with_diagnostics(const message_20::AC_ChargeParameterDiscoveryRequest& req, const d20::Session& session,
                                const d20::AcTransferLimits& limits, const d20::AcPresentPower& powers,
                                const d20::IAcDerControlProvider& ac_der_control_provider);

} // namespace iso15118::d20::state

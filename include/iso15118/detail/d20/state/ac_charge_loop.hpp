// SPDX-License-Identifier: Apache-2.0
// Copyright 205 Pionix GmbH and Contributors to EVerest
#pragma once

#include <iso15118/d20/session.hpp>
#include <iso15118/message/ac_charge_loop.hpp>

#include <iso15118/d20/ac_powers.hpp>
#include <iso15118/d20/ac_der_control.hpp>
#include <iso15118/d20/dynamic_mode_parameters.hpp>
#include <iso15118/d20/limits.hpp>

namespace iso15118::d20::state {

message_20::AC_ChargeLoopResponse handle_request(const message_20::AC_ChargeLoopRequest& req,
                                                 const d20::Session& session, bool stop, bool pause,
                                                 float target_frequency, const AcTargetPower& target_powers,
                                                 const AcPresentPower& present_powers,
                                                 const UpdateDynamicModeParameters& dynamic_parameters);

message_20::AC_ChargeLoopResponse handle_request(const message_20::AC_ChargeLoopRequest& req,
                                                 const d20::Session& session, bool stop, bool pause,
                                                 float target_frequency, const d20::AcTransferLimits& ac_limits,
                                                 const AcTargetPower& target_powers,
                                                 const AcPresentPower& present_powers,
                                                 const UpdateDynamicModeParameters& dynamic_parameters);

message_20::AC_ChargeLoopResponse handle_request(const message_20::AC_ChargeLoopRequest& req,
                                                 const d20::Session& session, bool stop, bool pause,
                                                 float target_frequency, const d20::AcTransferLimits& ac_limits,
                                                 const AcTargetPower& target_powers,
                                                 const AcPresentPower& present_powers,
                                                 const UpdateDynamicModeParameters& dynamic_parameters,
                                                 const AcDerControlConfig& ac_der_control_config);

} // namespace iso15118::d20::state

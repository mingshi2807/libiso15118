# AC DER IEC Standards Traceability

This matrix links the current AC_DER_IEC Dynamic EIM implementation to the local ISO 15118-20 AMD1 FDIS/FDAM knowledge
slice. It is metadata-only: keep private standards text in the knowledge base, not in this repository.

Source baseline:

- `source_id`: `ISO15118-20-AMD1`
- `source_uri`: `knowledge/raw/ISO15118-20/iso15118-20amd1_fdis.pdf`
- extracted AC_DER pages: `26-41`, `146-189`, `190-226`
- local knowledge status at implementation time: `724` AMD1 AC_DER chunks and `724` local embeddings

## Scope

In scope for this implementation:

- AC_DER_IEC selected service on the SECC side
- Dynamic control mode with EVCC-provided mobility needs
- EIM authorization path in the production-facing demo
- DER control functions required by the current client profile:
  - `VoltWatt`
  - `DSOQSetPointProvision`
  - `DSOQCosphiSetPointProvision`
  - `DCInjectionRestriction`
  - `OverFrequencyWatt`
  - `UnderFrequencyWatt`
  - `VoltVar`
  - `WattVar`
  - `WattCosPhi`
  - `OverVoltageFaultRideThrough`
  - `UnderVoltageFaultRideThrough`
  - `ZeroCurrent`

Out of scope for this implementation:

- AC_DER_IEC scheduled control mode execution
- SAE AC DER profile
- MCS, MCS_BPT, and cybersecurity additions from AMD1
- live DSO/backend integration; the library receives already validated application snapshots

## Traceability Matrix

| Standard anchor | Chunk id | Implementation anchor | Test anchor | Current status | Known limitation |
|---|---|---|---|---|---|
| `8.3.4.10` AC DER messages | `ISO15118-20-AMD1:pages-26-41:000042` | `src/iso15118/message/ac_charge_parameter_discovery.cpp`, `src/iso15118/message/ac_charge_loop.cpp` | `test/exi/cb/iso20/ac_charge_parameter_discovery.cpp`, `test/exi/cb/iso20/ac_charge_loop.cpp` | DER AC message variants are encoded and decoded through the existing EXI conversion layer. | Broader AMD1 message families outside AC_DER_IEC are not part of this slice. |
| `8.4.3.2.9` AC DER service | `ISO15118-20-AMD1:pages-26-41:000141` | `src/iso15118/d20/state/service_discovery.cpp`, `src/iso15118/d20/state/service_selection.cpp` | `test/iso15118/states/service_discovery.cpp`, `test/iso15118/states/service_selection.cpp` | AC_DER can be offered, selected, and kept separate from AC_BPT. | Coexistence is supported as advertised services; one selected energy-transfer service drives each active session path. |
| `L.3.1.1` Service parameters for AC DER IEC | `ISO15118-20-AMD1:pages-146-189:000252` | `src/iso15118/d20/state/service_detail.cpp`, `src/iso15118/message/service_detail.cpp` | `test/iso15118/states/service_detail.cpp`, `test/exi/cb/iso20/service_detail.cpp` | ServiceDetail exposes AC_DER parameters and DER control-function metadata. | Parameter semantics are validated at the supported bitmap and provider-contract level, not as a full external conformance suite. |
| `L.3.1.1.2` AC DER IEC service and bitmap | `ISO15118-20-AMD1:pages-146-189:000255`, `ISO15118-20-AMD1:pages-146-189:000257`, `ISO15118-20-AMD1:pages-146-189:000263` | `src/iso15118/d20/config.cpp`, `src/iso15118/d20/ac_der_control.cpp` | `test/iso15118/d20/ac_der_control.cpp`, `test/iso15118/states/service_detail.cpp`, `test/iso15118/states/service_selection.cpp` | Mandatory client-profile controls are centralized and rejected when selected or supported capability is incomplete. | Bitmap constants are represented in the current library `DERControlFunctions` split fields; verify against schema regeneration if the generated model changes. |
| `L.3.1.1.2.1` DERControlFunctionsBitmap | `ISO15118-20-AMD1:pages-146-189:000266`, `ISO15118-20-AMD1:pages-146-189:000267`, `ISO15118-20-AMD1:pages-146-189:000269` | `src/iso15118/d20/ac_der_control.cpp::has_required_ac_der_control_functions`, `src/iso15118/d20/ac_der_control.cpp::validate_ac_der_secc_control_snapshots` | `test/iso15118/d20/ac_der_control.cpp` | All required bitmap-backed control functions are checked individually in positive and missing-bit negative tests. | Current SECC target requires all listed client-profile functions; future profiles may need configurable subsets. |
| `L.2.1.1.2` AC_ChargeParameterDiscoveryReq/Res for AC DER IEC | `ISO15118-20-AMD1:pages-146-189:000031` | `src/iso15118/d20/state/ac_charge_parameter_discovery.cpp` | `test/iso15118/states/ac_charge_parameter_discovery.cpp` | AC_DER CPD validates selected service compatibility, provider availability, and Dynamic scope before emitting DER response data. | Scheduled AC_DER is rejected in the current provider scope. |
| `L.2.1.1.2.1` AC_ChargeParameterDiscoveryReq for AC DER IEC | `ISO15118-20-AMD1:pages-146-189:000036`, `ISO15118-20-AMD1:pages-146-189:000037` | `src/iso15118/message/ac_charge_parameter_discovery.cpp` | `test/exi/cb/iso20/ac_charge_parameter_discovery.cpp` | DER AC CPD request data model is converted between generated codec structs and library datatypes. | Request-side policy semantics remain owned by the application and conformance tests. |
| `L.2.1.1.2.2` AC_ChargeParameterDiscoveryRes for AC DER IEC | `ISO15118-20-AMD1:pages-146-189:000043`, `ISO15118-20-AMD1:pages-146-189:000044` | `src/iso15118/d20/state/ac_charge_parameter_discovery.cpp`, `src/iso15118/message/ac_charge_parameter_discovery.cpp` | `test/iso15118/states/ac_charge_parameter_discovery.cpp`, `test/exi/cb/iso20/ac_charge_parameter_discovery.cpp` | SECC response carries DER AC transfer-mode payloads from `IAcDerControlProvider`. | Full grid-code-specific parameter policy is intentionally outside the protocol stack. |
| `L.2.1.1.3` AC_ChargeLoopReq/Res for AC DER IEC | `ISO15118-20-AMD1:pages-146-189:000050` | `src/iso15118/d20/state/ac_charge_loop.cpp` | `test/iso15118/states/ac_charge_loop.cpp` | AC_DER ChargeLoop validates selected service, provider availability, Dynamic scope, and selected control functions. | Scheduled AC_DER request/response behavior is not implemented beyond explicit rejection in this profile. |
| `L.2.1.1.3.1` AC_ChargeLoopReq for AC DER IEC | `ISO15118-20-AMD1:pages-146-189:000052`, `ISO15118-20-AMD1:pages-146-189:000053` | `src/iso15118/message/ac_charge_loop.cpp` | `test/exi/cb/iso20/ac_charge_loop.cpp` | DER Dynamic and Scheduled request control-mode variants are decoded into library datatypes. | Scheduled provider semantics are deferred. |
| `L.2.1.1.3.2` AC_ChargeLoopRes for AC DER IEC | `ISO15118-20-AMD1:pages-146-189:000058`, `ISO15118-20-AMD1:pages-146-189:000059` | `src/iso15118/d20/state/ac_charge_loop.cpp`, `src/iso15118/message/ac_charge_loop.cpp` | `test/iso15118/states/ac_charge_loop.cpp`, `test/exi/cb/iso20/ac_charge_loop.cpp` | DER Dynamic response values are populated from provider configuration and converted for EXI output. | Response value validation is limited to implemented control structures and basic numeric consistency. |
| `L.2.2.1.2` DER_AC_CPDReqEnergyTransferModeType | `ISO15118-20-AMD1:pages-146-189:000080`, `ISO15118-20-AMD1:pages-146-189:000081` | `include/iso15118/message/ac_charge_parameter_discovery.hpp`, `src/iso15118/message/ac_charge_parameter_discovery.cpp` | `test/exi/cb/iso20/ac_charge_parameter_discovery.cpp` | DER AC CPD request type exists and is round-trip covered. | Field-level conformance should be expanded with official test vectors when available. |
| `L.2.2.1.4` DER_Dynamic_AC_CLReqControlModeType | `ISO15118-20-AMD1:pages-146-189:000101`, `ISO15118-20-AMD1:pages-146-189:000102` | `include/iso15118/message/ac_charge_loop.hpp`, `src/iso15118/message/ac_charge_loop.cpp` | `test/exi/cb/iso20/ac_charge_loop.cpp`, `test/iso15118/states/ac_charge_loop.cpp` | Dynamic DER AC request control mode is represented and accepted when AC_DER Dynamic is selected. | EV-side control activation behavior is outside this SECC-focused integration. |
| `L.2.2.1.5` DER_Scheduled_AC_CLReqControlModeType | `ISO15118-20-AMD1:pages-146-189:000116`, `ISO15118-20-AMD1:pages-146-189:000117` | `include/iso15118/message/ac_charge_loop.hpp`, `src/iso15118/d20/state/ac_charge_loop.cpp` | `test/iso15118/states/ac_charge_loop.cpp`, `test/exi/cb/iso20/ac_charge_loop.cpp` | Scheduled request type is represented, but rejected by the AC_DER_IEC Dynamic EIM provider scope. | Implement scheduled AC_DER only after a concrete SECC application requirement exists. |
| `L.2.2.1.6` DER_Dynamic_AC_CLResControlModeType | `ISO15118-20-AMD1:pages-146-189:000126` | `include/iso15118/message/ac_charge_loop.hpp`, `src/iso15118/d20/state/ac_charge_loop.cpp` | `test/iso15118/states/ac_charge_loop.cpp`, `test/exi/cb/iso20/ac_charge_loop.cpp` | Dynamic DER AC response is populated with DSOQ and DSOCosPhi provider data. | Advanced feedback/status handling remains future work. |
| `L.2.2.1.7` DER_Scheduled_AC_CLResControlModeType | `ISO15118-20-AMD1:pages-146-189:000138` | `include/iso15118/message/ac_charge_loop.hpp`, `src/iso15118/message/ac_charge_loop.cpp` | `test/exi/cb/iso20/ac_charge_loop.cpp` | Scheduled response datatype is represented in the codec layer. | SECC scheduled control generation is deferred. |
| `L.2.2.1.8` DERControlType | `ISO15118-20-AMD1:pages-146-189:000147` | `include/iso15118/message/ac_charge_parameter_discovery.hpp`, `src/iso15118/d20/ac_der_control.cpp` | `test/iso15118/d20/ac_der_control.cpp`, `test/iso15118/states/ac_charge_parameter_discovery.cpp` | Provider-generated CPD control payload includes active power, reactive power, DC injection, fault ride-through, and zero-current structures. | Values are application snapshots, not internally computed grid-code policy. |
| `L.2.2.1.9` FaultRideThroughType | `ISO15118-20-AMD1:pages-146-189:000153` | `src/iso15118/d20/ac_der_control.cpp::validate_ac_der_control_config` | `test/iso15118/d20/ac_der_control.cpp`, `test/iso15118/states/ac_charge_loop.cpp` | Over-voltage and under-voltage fault ride-through are required and validated for presence and basic numeric shape. | Detailed ride-through curves and grid-code timing rules remain application/conformance-layer concerns. |
| `L.2.2.1.10` ZeroCurrentType | `ISO15118-20-AMD1:pages-146-189:000159` | `src/iso15118/d20/ac_der_control.cpp::validate_ac_der_control_config` | `test/iso15118/d20/ac_der_control.cpp` | ZeroCurrent is required and validated for at least one voltage limit plus non-negative timing. | Detailed deadband/lock behavior is not modeled in this SECC provider slice. |
| `L.2.2.1.11` ReactivePowerSupportType | `ISO15118-20-AMD1:pages-146-189:000166`, `ISO15118-20-AMD1:pages-146-189:000170` | `src/iso15118/d20/ac_der_control.cpp::make_default_ac_der_control_config`, `src/iso15118/d20/ac_der_control.cpp::validate_ac_der_control_config` | `test/iso15118/d20/ac_der_control.cpp`, `test/iso15118/states/ac_charge_loop.cpp` | VoltVar, WattVar, and WattCosPhi payload presence and curve shape are validated when negotiated. | Curve point limits are basic monotonic checks; detailed profile constraints should be added from conformance vectors. |
| `L.2.2.1.12` ActivePowerSupportType | `ISO15118-20-AMD1:pages-146-189:000171` | `src/iso15118/d20/ac_der_control.cpp` | `test/iso15118/d20/ac_der_control.cpp`, `test/iso15118/states/ac_charge_parameter_discovery.cpp` | VoltWatt, under-frequency watt, and over-frequency watt payloads are produced from grid-policy snapshots. | Frequency thresholds are validated for direction and positivity only. |
| `L.2.2.1.13` DERCurveType | `ISO15118-20-AMD1:pages-146-189:000177` | `src/iso15118/d20/ac_der_control.cpp::validate_der_curve` | `test/iso15118/d20/ac_der_control.cpp` | DER curve structures require at least two points and monotonic x-axis values. | Unit-specific and profile-specific curve bounds remain future conformance work. |
| `L.2.2.1.17` FrequencyWattType | `ISO15118-20-AMD1:pages-146-189:000200` | `src/iso15118/d20/ac_der_control.cpp::validate_frequency_watt` | `test/iso15118/d20/ac_der_control.cpp` | Under- and over-frequency watt modes are validated with direction-specific frequency thresholds. | Full frequency droop policy remains owned by the application/grid-policy layer. |
| `L.2.2.1.18` VoltWattType | `ISO15118-20-AMD1:pages-146-189:000206`, `ISO15118-20-AMD1:pages-146-189:000211` | `src/iso15118/d20/ac_der_control.cpp::validate_volt_watt`, `examples/ac_der_secc_application_adapter.cpp` | `test/iso15118/d20/ac_der_control.cpp`, `test/iso15118/d20/ac_der_secc_application_adapter.cpp` | VoltWatt start/stop voltage is populated from SECC grid policy and rejected when reversed. | More detailed voltage curve behavior should be validated once client grid-code vectors are available. |
| `L.2.2.1.20` DSOQSetpointType | `ISO15118-20-AMD1:pages-146-189:000218`, `ISO15118-20-AMD1:pages-146-189:000222` | `src/iso15118/d20/ac_der_control.cpp::validate_dso_q_setpoint`, `src/iso15118/d20/state/ac_charge_loop.cpp` | `test/iso15118/d20/ac_der_control.cpp`, `test/iso15118/states/ac_charge_loop.cpp` | DSOQ setpoint is carried into Dynamic AC_DER ChargeLoop response and validated for non-negative response timing. | Sign-convention and per-phase policy interpretation must stay with application/grid-code integration. |
| `L.4.4.2.1` SECC message flow for AC DER IEC | `ISO15118-20-AMD1:pages-146-189:000292` | `src/iso15118/d20/state/ac_charge_parameter_discovery.cpp`, `test/iso15118/fsm/ac_der_iec_flow.cpp` | `test/iso15118/fsm/ac_der_iec_flow.cpp` | FSM integration test covers SupportedAppProtocol through AC_DER CPD in the current Dynamic EIM path. | Full production conformance timing and negative message-flow cases remain outside this library-level slice. |

## Review Gates

Before claiming AC_DER_IEC readiness, rerun:

```bash
cmake --build build-pin-der --target test_ac_der_control test_ac_der_secc_application_adapter test_service_detail test_service_selection test_ac_charge_parameter_discovery test_ac_charge_loop test_ac_der_iec_flow
./build-pin-der/test/iso15118/d20/test_ac_der_control
./build-pin-der/test/iso15118/d20/test_ac_der_secc_application_adapter
./build-pin-der/test/iso15118/states/test_service_detail
./build-pin-der/test/iso15118/states/test_service_selection
./build-pin-der/test/iso15118/states/test_ac_charge_parameter_discovery
./build-pin-der/test/iso15118/states/test_ac_charge_loop
./build-pin-der/test/iso15118/fsm/test_ac_der_iec_flow
ctest --test-dir build-pin-der --output-on-failure
```

When the local standards database is available, refresh the metadata anchors with:

```bash
standards_mcp.search_standards_reranked(
  query="AC DER IEC Dynamic EIM ServiceDetail ServiceSelection ChargeParameterDiscovery ChargeLoop DER control functions",
  source_id="ISO15118-20-AMD1",
  profile="ac_der_iec"
)
```

Do not paste standards body text into this file. Only update `source_id`, `section`, `heading`, `page_range`, and
`chunk_id` metadata.

# AC DER IEC Expert-Review Acceptance Pack

This pack is the handoff checklist for expert review of the ISO 15118-20 AMD1 AC_DER_IEC Dynamic EIM SECC
implementation. It is intentionally metadata-only for standards evidence: do not paste private standards text here.

## Review Target

Review the implementation as a production-integration candidate for this scope:

- ISO 15118-20 AMD1 `AC_DER_IEC` selected service on the SECC side
- Dynamic control mode
- EIM authorization path
- EVCC-provided mobility-needs mode
- selected DER control functions required by the current client profile:
  - `VoltWatt`
  - `DSOQSetPointProvision`
  - `DSOQCosphiSetPointProvision`
  - `DCInjectionRestriction`
  - `UnderFrequencyWatt`
  - `OverFrequencyWatt`
  - `VoltVar`
  - `WattVar`
  - `WattCosPhi`
  - `OverVoltageFaultRideThrough`
  - `UnderVoltageFaultRideThrough`
  - `ZeroCurrent`

Out of scope for this acceptance decision:

- SAE AC DER profile
- AC_DER scheduled control execution
- MCS, MCS_BPT, and cybersecurity AMD1 additions
- live DSO backend integration
- grid-code-specific policy calculation inside the protocol stack

## Evidence To Review

| Evidence | Location or command | Acceptance intent |
|---|---|---|
| Standards traceability | `docs/ac_der_iec_traceability.md` | Confirms every accepted feature has source metadata, implementation anchors, test anchors, status, and limitations. |
| Latest readiness review | `docs/ac_der_iec_readiness_review.md` | Records the fresh RAG, code-review, build, test, coverage, and readiness-score evidence for the current review gate. |
| SECC provider contract | `docs/ac_der_secc_provider.md` | Confirms the application-layer API, ownership boundary, runtime behavior, and demo integration path are explicit. |
| Readiness gate | `tools/ac_der_iec_readiness.sh` | Builds focused targets, runs focused AC_DER_IEC tests, runs demos, then runs full CTest. |
| Focused coverage gate | `tools/ac_der_iec_coverage.sh` | Produces AC_DER_IEC coverage reports and enforces the focused line-coverage threshold. |
| Provider/control tests | `test/iso15118/d20/ac_der_control.cpp`, `test/iso15118/d20/ac_der_secc_application_adapter.cpp` | Confirms bitmap consistency, provider validation, snapshot adapter behavior, and negative paths. |
| State-machine tests | `test/iso15118/states/ac_charge_parameter_discovery.cpp`, `test/iso15118/states/ac_charge_loop.cpp`, `test/iso15118/fsm/ac_der_iec_flow.cpp` | Confirms CPD, ChargeLoop, and main FSM integration behavior. |
| EXI conversion tests | `test/exi/cb/iso20/ac_charge_parameter_discovery.cpp`, `test/exi/cb/iso20/ac_charge_loop.cpp`, `test/exi/cb/iso20/service_detail.cpp`, `test/exi/cb/iso20/service_selection.cpp` | Confirms generated codec integration and AC_DER message conversion behavior. |
| Production-facing demos | `examples/ac_der_secc_provider.cpp`, `examples/ac_der_secc_application_adapter.cpp` | Confirms a SECC application can inject policy snapshots and observe accepted/rejected contexts. |

## Evidence Commands

Run from the repository root:

```bash
tools/ac_der_iec_readiness.sh
tools/ac_der_iec_coverage.sh
```

The coverage command writes review artifacts to `build-pin-der/ac_der_coverage/`:

- `ac_der_iec_coverage.txt`
- `ac_der_iec_coverage.xml`
- `ac_der_iec_coverage_summary.json`
- `html/index.html`

## Acceptance Checklist

Mark each row as `Pass`, `Fail`, or `N/A with rationale`.

| Area | Acceptance criterion | Reviewer result | Notes |
|---|---|---|---|
| Scope control | Implementation is limited to AC_DER_IEC Dynamic EIM and does not claim SAE, scheduled AC_DER, MCS, or cybersecurity support. |  |  |
| Service separation | AC_DER is offered/selected distinctly from AC_BPT; active CPD/ChargeLoop behavior follows the selected service. |  |  |
| ServiceDetail | AC_DER service parameters and DER control-function metadata are available and test-covered. |  |  |
| Mandatory bitmap | All required client-profile DER control functions are supported and selected consistently. |  |  |
| CPD request/response | AC_DER CPD request and response paths are represented, converted, and validated through tests. |  |  |
| ChargeLoop request/response | Dynamic AC_DER ChargeLoop request and response paths are represented, converted, and validated through tests. |  |  |
| Provider contract | SECC application owns policy data; protocol stack consumes immutable or synchronized snapshots through `IAcDerControlProvider`. |  |  |
| Runtime diagnostics | Unsupported service, mode, stale policy, invalid capability, or incomplete selection returns a diagnosable failure path. |  |  |
| Control validation | VoltWatt, FrequencyWatt, reactive-power support, DSO setpoints, DC injection, ride-through, and zero-current structures have basic validation. |  |  |
| Negative paths | Missing mandatory capabilities, invalid values, unsupported mode, and AC_BPT context rejection are covered by tests. |  |  |
| Codec integration | AC_DER data models round-trip through generated codec conversion tests for CPD, ChargeLoop, ServiceDetail, and ServiceSelection. |  |  |
| FSM integration | SupportedAppProtocol-to-AC_DER CPD path is covered in the FSM test for the Dynamic EIM scenario. |  |  |
| Coverage evidence | `tools/ac_der_iec_coverage.sh` passes and the generated text report is attached to the review. |  |  |
| Traceability | `docs/ac_der_iec_traceability.md` has no unsupported normative claims and all anchors use metadata only. |  |  |
| Production integration | Demo adapter is realistic enough for SECC application integration planning and does not hide client policy in defaults. |  |  |

## Release-Blocking Findings

Record any finding that prevents production-candidate acceptance:

| Finding id | Severity | Evidence anchor | Required fix | Owner | Status |
|---|---|---|---|---|---|
|  |  |  |  |  |  |

Severity guidance:

- `Blocker`: violates selected-service behavior, mandatory control support, safety-critical validation, or build/test gate.
- `High`: accepted behavior is ambiguous, weakly tested, or hard to integrate safely.
- `Medium`: documentation, diagnostics, or traceability is incomplete but implementation behavior is acceptable.
- `Low`: cleanup or clarification that does not affect acceptance.

## Expert Sign-Off

The feature can be marked `expert-reviewed production candidate` only when:

- all in-scope checklist rows are `Pass`
- all `Blocker` and `High` findings are closed
- remaining `Medium` findings are explicitly accepted with owner and follow-up
- readiness and focused coverage gates pass on the review build
- traceability limitations are accepted by the protocol owner

Sign-off record:

| Role | Name | Decision | Date | Notes |
|---|---|---|---|---|
| Protocol expert |  |  |  |  |
| SECC application owner |  |  |  |  |
| Test/conformance owner |  |  |  |  |
| Release owner |  |  |  |  |

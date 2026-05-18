# AC DER IEC Readiness Review

This review records the fresh step 11 evidence for the current ISO 15118-20 AMD1 `AC_DER_IEC` Dynamic EIM SECC
implementation. It is metadata-only for standards evidence: do not paste private standards text here.

Review date: 2026-05-18

## Scope

In scope:

- `AC_DER_IEC` selected service on the SECC side
- Dynamic control mode
- EIM authorization path
- EVCC-provided mobility-needs mode
- mandatory client-profile controls: `VoltWatt`, `DSOQSetPointProvision`, `DSOQCosphiSetPointProvision`,
  `DCInjectionRestriction`, `UnderFrequencyWatt`, `OverFrequencyWatt`, `VoltVar`, `WattVar`, `WattCosPhi`,
  `OverVoltageFaultRideThrough`, `UnderVoltageFaultRideThrough`, and `ZeroCurrent`

Out of scope:

- SAE AC DER profile
- scheduled AC_DER control execution
- MCS, MCS_BPT, and cybersecurity AMD1 additions
- live DSO backend integration
- grid-code-specific policy calculation inside the protocol stack

## Standards Retrieval Gate

Fresh `standards_mcp.search_standards_hybrid` checks found the required AC_DER_IEC anchors in the local AMD1 FDIS
knowledge source:

| Anchor | Source id | Section | Heading | Page range | Chunk id |
|---|---|---|---|---|---|
| Service definition | `ISO15118-20-AMD1` | `8.4.3.2.9` | `AC DER service` | `35-35` | `ISO15118-20-AMD1:pages-26-41:000141` |
| Service parameters | `ISO15118-20-AMD1` | `L.3.1.1` | `Service parameters for the AC DER IEC service` | `185-185` | `ISO15118-20-AMD1:pages-146-189:000252` |
| Service bitmap | `ISO15118-20-AMD1` | `L.3.1.1.2` | `AC DER IEC service and bitmap` | `186-186` | `ISO15118-20-AMD1:pages-146-189:000256` |
| Control-function bitmap | `ISO15118-20-AMD1` | `L.3.1.1.2.1` | `DERControlFunctionsBitmap` | `187-187` | `ISO15118-20-AMD1:pages-146-189:000267` |
| CPD request/response | `ISO15118-20-AMD1` | `L.2.1.1.2` | `AC_ChargeParameterDiscoveryReq/Res for the AC DER IEC service` | `148-148` | `ISO15118-20-AMD1:pages-146-189:000031` |
| CPD request type | `ISO15118-20-AMD1` | `L.2.2.1.2` | `DER_AC_CPDReqEnergyTransferModeType` | `153-153` | `ISO15118-20-AMD1:pages-146-189:000081` |
| CPD response type | `ISO15118-20-AMD1` | `L.2.2.1.3` | `DER_AC_CPDResEnergyTransferModeType` | `155-155` | `ISO15118-20-AMD1:pages-146-189:000092` |
| ChargeLoop request/response | `ISO15118-20-AMD1` | `L.2.1.1.3` | `AC_ChargeLoopReq/Res for the AC DER IEC service` | `150-150` | `ISO15118-20-AMD1:pages-146-189:000050` |
| Dynamic ChargeLoop request | `ISO15118-20-AMD1` | `L.2.2.1.4` | `DER_Dynamic_AC_CLReqControlModeType` | `158-158` | `ISO15118-20-AMD1:pages-146-189:000102` |
| Dynamic ChargeLoop response | `ISO15118-20-AMD1` | `L.2.2.1.6` | `DER_Dynamic_AC_CLResControlModeType` | `161-161` | `ISO15118-20-AMD1:pages-146-189:000127` |
| Fault ride-through | `ISO15118-20-AMD1` | `L.2.2.1.9` | `FaultRideThroughType` | `165-165` | `ISO15118-20-AMD1:pages-146-189:000154` |
| Reactive power support | `ISO15118-20-AMD1` | `L.2.2.1.11` | `ReactivePowerSupportType` | `168-168` | `ISO15118-20-AMD1:pages-146-189:000167` |
| Active power support | `ISO15118-20-AMD1` | `L.2.2.1.12` | `ActivePowerSupportType` | `169-169` | `ISO15118-20-AMD1:pages-146-189:000172` |
| FrequencyWatt | `ISO15118-20-AMD1` | `L.2.2.1.17` | `FrequencyWattType` | `176-176` | `ISO15118-20-AMD1:pages-146-189:000201` |
| VoltWatt | `ISO15118-20-AMD1` | `L.2.2.1.18` | `VoltWattType` | `177-177` | `ISO15118-20-AMD1:pages-146-189:000207` |
| DSOQ setpoint | `ISO15118-20-AMD1` | `L.2.2.1.20` | `DSOQSetpointType` | `180-180` | `ISO15118-20-AMD1:pages-146-189:000218` |

Retrieval status at review time:

- database: `standards`
- database size: `91 MB`
- sources: `2`
- chunks: `5364`
- embedded chunks: `5364`
- missing embeddings: `0`
- embedding model: `local:BAAI/bge-m3:1024`

## Implementation Review Gate

Findings:

- `Blocker`: none found in the inspected `AC_DER_IEC` Dynamic EIM scope.
- `High`: none found in the inspected `AC_DER_IEC` Dynamic EIM scope.
- `Medium`: none found in the inspected `AC_DER_IEC` Dynamic EIM scope.
- `Watch`: broad RAG queries can still return lower-ranked SAE Annex M chunks; reviewers should prefer the IEC profile query and AMD1 Annex L anchors.
- `Watch`: scheduled AC_DER request/response datatypes are represented, but execution remains out of scope and is rejected by the production provider policy.
- `Watch`: `StaticAcDerControlProvider` is a compatibility/demo helper; production SECC integration should use `make_secc_ac_der_control_provider()`.

Inspected implementation coverage:

- Service discovery and selection keep `AC_DER` separate from `AC_BPT`.
- ServiceDetail exposes AC_DER service parameters and mandatory DER control-function metadata.
- CPD validates selected service compatibility, mandatory controls, provider availability, Dynamic scope, and provider output.
- ChargeLoop validates selected service compatibility, mandatory controls, provider availability, Dynamic scope, and provider output.
- The SECC provider rejects unsupported mode, unsupported mobility-needs mode, stale policy, invalid policy, invalid DSO control,
  invalid capability, unsupported selected controls, and incomplete mandatory selections.
- Control validation covers VoltWatt, under/over FrequencyWatt, VoltVar, WattVar, WattCosPhi, DSOQ setpoint,
  DSOCosPhi setpoint, DC injection, over/under voltage fault ride-through, and ZeroCurrent.

## Build, Test, And Coverage Gate

Fresh local evidence:

```text
cmake --build build-pin-der
Result: passed
```

```text
ctest --test-dir build-pin-der --output-on-failure
Result: 43/43 tests passed
```

```text
tools/ac_der_iec_coverage.sh
Result: passed
Focused line coverage: 82.1% (2122 / 2584)
Focused branch coverage: 43.7% (1578 / 3615)
Threshold: 70% focused line coverage
```

Coverage artifacts:

- `build-pin-der/ac_der_coverage/ac_der_iec_coverage.txt`
- `build-pin-der/ac_der_coverage/ac_der_iec_coverage.xml`
- `build-pin-der/ac_der_coverage/ac_der_iec_coverage_summary.json`
- `build-pin-der/ac_der_coverage/html/index.html`

## Readiness Scores

Scores are based only on the fresh evidence above.

| Area | Score | Rationale |
|---|---:|---|
| Standards retrieval and traceability | `9/10` | Mandatory IEC AC_DER anchors are retrievable with source metadata and no missing embeddings. Lower-ranked SAE noise remains for broad queries. |
| Implementation coverage | `9/10` | In-scope Dynamic EIM SECC paths are implemented across service discovery, ServiceDetail, ServiceSelection, CPD, ChargeLoop, provider contract, diagnostics, and control validation. Scheduled execution and SAE remain out of scope. |
| Build/test/coverage reliability | `10/10` | Clean build passed, full CTest passed, readiness and coverage gates passed, and focused line coverage is above threshold. |
| Production SECC integration readiness | `9/10` | Provider contract, demo adapter, diagnostics, and negative tests are present. Live DSO integration and grid-code conformance vectors remain outside this library slice. |
| Expert-review readiness | `9/10` | Acceptance pack, traceability, fresh RAG anchors, tests, coverage, and known limitations are available. Human expert sign-off is still required. |

Overall status: `expert-review candidate`, not yet `expert-granted production candidate`.

## Stop Condition

Step 11 is complete when this file, `docs/ac_der_iec_traceability.md`, and `docs/ac_der_iec_acceptance_pack.md` are reviewed
together with the fresh build/test/coverage artifacts above. The readiness score must not be raised to `10/10` until the
expert-review checklist is signed off and any required conformance vectors are accepted or added.

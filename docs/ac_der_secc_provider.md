# AC DER SECC Provider Integration

`IAcDerControlProvider` is the SECC application-layer contract for ISO 15118-20 AMD1 AC_DER IEC control data.

The protocol state machine owns message sequencing. The SECC application owns the DER policy:

- grid or DSO commands
- EVSE capability limits
- VoltWatt, FrequencyWatt, VoltVar, WattVar, and WattCosPhi policy values
- DSOQ and DSOCosPhi setpoints
- DC injection restriction
- fallback behavior when AC_DER is selected but policy data is unavailable

## API Contract

Implement:

```cpp
class MyAcDerControlProvider : public iso15118::d20::IAcDerControlProvider {
public:
    std::optional<iso15118::d20::AcDerControlConfig>
    get_ac_der_control_config(const iso15118::d20::AcDerControlContext& context) const override;
};
```

The provider receives the selected session context:

- `selected_energy_service`
- `selected_control_mode`
- `selected_mobility_needs_mode`
- `selected_der_control_functions`

Return `std::nullopt` when the current SECC application state cannot support AC_DER. When `AC_DER` is selected, this makes ChargeParameterDiscovery or ChargeLoop fail explicitly instead of silently sending incomplete DER data.

For the first production integration shape, the library also provides a snapshot adapter:

```cpp
auto snapshots = iso15118::d20::make_default_ac_der_secc_control_snapshots();
snapshots.grid_policy.volt_watt_start_voltage = ...;
snapshots.grid_policy.maximum_dc_injection = ...;
snapshots.dso_control.q_setpoint = ...;
snapshots.dso_control.cos_phi_setpoint = ...;

auto provider = iso15118::d20::make_secc_ac_der_control_provider(snapshots);
```

Use `AcDerSeccControlSnapshots` as the boundary between the SECC application and the protocol stack:

- `AcDerGridPolicySnapshot`: VoltWatt, FrequencyWatt, and DC injection restriction values
- `AcDerDsoControlSnapshot`: DSOQ and DSOCosPhi setpoints
- `AcDerEvseCapabilitySnapshot`: supported AC_DER IEC control-function bitmap
- `AcDerRuntimeStateSnapshot`: AC_DER enablement and freshness gates

The adapter returns `std::nullopt` if the selected service is not AC_DER, selected DER functions are missing, policy snapshots are invalid/stale, or the EVCC-selected functions exceed EVSE capability.

## Session Injection

Inject the provider through `EvseSetupConfig`:

```cpp
auto provider = std::make_shared<MyAcDerControlProvider>(...);
auto setup = iso15118::d20::EvseSetupConfig{...};
setup.ac_der_control_provider = provider;

auto session_config = iso15118::d20::SessionConfig(setup);
```

The provider is stored as `std::shared_ptr<const IAcDerControlProvider>`. It must remain valid for the full protocol session. Use immutable snapshots or internal synchronization if the provider reads live DSO, EVSE, or grid state.

## Runtime Behavior

- `AC_ChargeParameterDiscovery` asks the provider for `AcDerControlConfig` and uses `cpd_control`.
- `AC_ChargeLoop` asks the provider for `AcDerControlConfig` and uses `dso_q_setpoint` and `dso_cos_phi_setpoint`.
- `AC_BPT` and plain `AC` request paths do not consume AC_DER provider data.
- `make_default_ac_der_control_config()` and `make_static_ac_der_control_provider()` are compatibility/demo helpers, not a production policy implementation.
- `make_secc_ac_der_control_provider()` is the preferred application-layer adapter for SECC integration tests and production-readiness demos.

## Compilable Example

See `examples/ac_der_secc_provider.cpp`.

With examples enabled:

```bash
cmake --build build-local-der --target example_ac_der_secc_provider
./build-local-der/examples/example_ac_der_secc_provider
```

## Production-Facing Adapter Example

See `examples/ac_der_secc_application_adapter.cpp` for a fuller SECC application-layer integration shape. It keeps
application concepts separate from protocol concepts:

- `GridCodePolicy`: VoltWatt, FrequencyWatt, and DC injection policy values
- `DsoControlCommand`: DSOQ and DSOCosPhi setpoints
- `EvseDerCapability`: supported AC_DER IEC control functions
- `RuntimeHealth`: enablement and freshness gates

The example translates those application inputs into `AcDerSeccControlSnapshots`, injects the provider into
`EvseSetupConfig`, and verifies that the provider accepts an `AC_DER` selected-service context while rejecting an
`AC_BPT` context.

```bash
cmake --build build-local-der --target example_ac_der_secc_application_adapter
./build-local-der/examples/example_ac_der_secc_application_adapter
```

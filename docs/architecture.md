# libiso15118 Architecture

## Component Overview

```mermaid
graph TD
    subgraph "Application Layer (SECC)"
        APP[Application Adapter]
        CB[Feedback Callbacks]
        AE[Control Events]
    end

    subgraph "Session Layer (iso15118::Session)"
        PM[PollManager]
        S[Session::poll]
        CEQ[ControlEventQueue]
        ME[MessageExchange]
        TO[Timeouts]
    end

    subgraph "Protocol Layer (d20)"
        FSM[FSM State Machine]
        CTX[d20::Context]
        CFG[SessionConfig]
        SESS[d20::Session]
    end

    subgraph "Transport Layer (io)"
        CONN[IConnection]
        CP[ConnectionPlain]
        CS[ConnectionSSL]
        SDP[SdpServer]
    end

    subgraph "Codec Layer (message_20)"
        VAR[Variant]
        EXI[EXI Serialize / Deserialize]
    end

    APP --> CB
    APP --> AE
    AE --> CEQ
    CEQ --> S
    S --> PM
    S --> ME
    S --> TO
    S --> FSM
    FSM --> CTX
    CTX --> CFG
    CTX --> SESS
    CTX --> CB
    S --> CONN
    CONN --> CP
    CONN --> CS
    SDP --> CONN
    ME --> EXI
    EXI --> VAR
```

## Event Loop Sequence

The core runtime loop. `PollManager` drives `Session::poll()`, which reads V2GTP packets,
feeds the FSM, serializes responses, and writes them back through the transport connection.

```mermaid
sequenceDiagram
    participant App as Application
    participant PM as PollManager
    participant S as Session::poll()
    participant Conn as IConnection
    participant FSM as FSM State Machine
    participant ME as MessageExchange
    participant EXI as EXI Codec

    loop Every ~50 ms
        App->>PM: poll(timeout_ms)
        PM->>S: NEW_DATA callback
        S->>Conn: read(buf, len)
        Conn-->>S: ReadResult{bytes_read}
        S->>S: read_single_sdp_packet()
        alt packet incomplete
            S-->>PM: return (wait for more data)
        else packet complete
            S->>EXI: decode SDP payload to Variant
            S->>ME: set_request(variant)
            S->>FSM: feed(V2GTP_MESSAGE)
            FSM->>FSM: current_state.feed(event)
            alt state handled
                FSM->>ME: ctx.respond(msg)
                ME->>EXI: serialize(msg) to response_buffer
                FSM-->>S: FeedResult (handled)
            else state unhandled
                FSM-->>S: FeedResult (unhandled)
                S->>S: ctx.session_stopped = true
            end
            S->>ME: check_and_clear_response()
            ME-->>S: got_response, size, payload_type
            opt response pending
                S->>Conn: write(response_buffer, size)
                S->>EXI: log EXI payload
            end
        end
        S-->>PM: next_session_event (wakeup time)
    end
```

## State Machine Transitions (ISO 15118-20)

```mermaid
stateDiagram-v2
    [*] --> SupportedAppProtocol
    SupportedAppProtocol --> SessionSetup : SupportedAppProtocolReq
    SessionSetup --> AuthorizationSetup : SessionSetupReq
    AuthorizationSetup --> Authorization : AuthorizationSetupReq
    Authorization --> ServiceDiscovery : AuthorizationReq
    ServiceDiscovery --> ServiceDetail : ServiceDiscoveryReq
    ServiceDetail --> ServiceSelection : ServiceDetailReq

    state ServiceSelection {
        [*] --> select_energy
        select_energy --> AC : AC / AC_BPT / AC_DER
        select_energy --> DC : DC / DC_BPT / MCS / MCS_BPT
    }

    ServiceSelection --> AC_ChargeParameterDiscovery : AC selected
    ServiceSelection --> DC_ChargeParameterDiscovery : DC selected
    ServiceSelection --> ScheduleExchange : Scheduled mode

    AC_ChargeParameterDiscovery --> AC_ChargeLoop : AC_CPD_Res
    AC_ChargeParameterDiscovery --> ScheduleExchange : Scheduled mode

    DC_ChargeParameterDiscovery --> DC_CableCheck : DC_CPD_Res
    DC_ChargeParameterDiscovery --> ScheduleExchange : Scheduled mode

    DC_CableCheck --> DC_PreCharge : CableCheck OK
    DC_PreCharge --> DC_ChargeLoop : PreCharge OK
    ScheduleExchange --> DC_CableCheck : Schedule OK
    ScheduleExchange --> AC_ChargeLoop : Schedule OK (AC)

    AC_ChargeLoop --> PowerDelivery : Stop requested
    DC_ChargeLoop --> PowerDelivery : Stop requested
    DC_ChargeLoop --> DC_WeldingDetection : Stop + DC

    PowerDelivery --> SessionStop : PowerDeliveryReq(Stop)
    DC_WeldingDetection --> SessionStop : WeldingDetection OK

    SessionStop --> [*]
```

## AC DER IEC Control Flow

See [AC DER SECC Provider Integration](ac_der_secc_provider.md) for the detailed API contract and
a visual sequence diagram of the SECC to library interaction during AC_DER charge parameter negotiation
and charge-loop DSO control.

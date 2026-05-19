# Repository Guidelines

> Visual architecture and sequence diagrams: **[docs/architecture.md](docs/architecture.md)**.
> AC DER IEC control flow: **[docs/ac_der_secc_provider.md](docs/ac_der_secc_provider.md)**.

## Project Structure & Module Organization
- `include/`: Public headers, with stable APIs under `include/iso15118`.
- `src/`: Library implementation (C++), organized by protocol areas (e.g., `d20`, `io`, `message`, `session`).
- `test/`: Unit and integration tests, grouped by subsystem (e.g., `test/iso15118`, `test/exi`, `test/v2gtp`).
- `input/`: ISO 15118-20 schema download support; controlled by CMake option `OPT_AUTODOWNLOAD_ISO20_SCHEMAS`.
- `build/`: Out-of-tree build artifacts (default CMake build directory).

## Build, Test, and Development Commands
- Configure (tests on, Ninja):
  `cmake -S . -B build -G Ninja -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
- Build:
  `ninja -C build`
- Run tests (CTest wrapper):
  `ninja -C build test`
- Coverage (requires `gcovr >= 8.2`):
  `ninja -C build iso15118_gcovr_coverage`
- Optional: disable warnings via `-DISO15118_COMPILE_OPTIONS_WARNING=""`.

## Coding Style & Naming Conventions
- Language: C++ with CMake; follow existing style in `src/` and `include/`.
- Indentation: 4 spaces, braces on the same line; namespaces and types use `iso15118::` naming.
- Files: use descriptive snake_case filenames (e.g., `dc_charge_loop.cpp`).
- No formatter is enforced; keep changes consistent with adjacent code.

## Testing Guidelines
- Framework: Catch2 (via CMake FetchContent when tests enabled).
- Tests live under `test/` and use subsystem directories for grouping.
- Prefer descriptive test file names that match the feature under test.
- Run `ninja -C build test` or `ctest --test-dir build` for local verification.

## Commit & Pull Request Guidelines
- Commit messages are imperative and concise, often sentence case (e.g., “Add offline dependency fallbacks for CMake build”).
- Include context or issue/PR reference in parentheses when helpful (e.g., “... (#162)”).
- PRs should include: purpose, key changes, test results (commands + outcome), and any relevant limitations.

## Configuration & Dependencies
- Requires OpenSSL 3+. Dependencies are resolved via EDM when available, otherwise fetched via CMake.
- Schema auto-download requires `wget` and accepting the ISO license; enable with `-DOPT_AUTODOWNLOAD_ISO20_SCHEMAS=ON`.

## Tech Stack Notes
- Language standard: C++17 (`target_compile_features(iso15118 PUBLIC cxx_std_17)`).
- Transport stack: IPv6 TCP sockets with optional TLS (OpenSSL), plus V2GTP framing and SDP server support.
- Event loop: `io::PollManager` uses `poll()` for fd-driven IO.
- EXI codec: `libcbv2g` provides EXI encode/decode and V2GTP helpers.
- Build system: CMake + `everest-cmake` helpers; FetchContent fallback for `libcbv2g` and Catch2.

## Networking & Security
- TLS negotiation is configured via `config::TlsNegotiationStrategy` (accept client offer, enforce TLS, or enforce no TLS).
- Certificate handling is configured in `config::SSLConfig` with EVEREST or JOSEPPA layouts.
- SDP server (`io::SdpServer`) handles IPv6 multicast discovery and returns the EVSE endpoint; `ConnectionSSL`/`ConnectionPlain` handle session transport.

## Protocol Coverage
- Focus is ISO 15118-20 message/state handling (AC/DC, scheduled/dynamic flows) implemented under `src/iso15118/d20` and `src/iso15118/message`.
- App-handshake and V2GTP framing are supported via `libcbv2g`.
- DIN70121 and ISO15118-2 are not in this repo; they live in EVerest modules (see README).

## Runtime Footprint & Concurrency
- Single-threaded polling loop by default; IO and session state progress via `PollManager::poll()` + `Session::poll()`.
- `Session::poll()` includes timeouts and may sleep briefly when terminating a session; consider this if integrating in a tight real-time loop.
- Callbacks in `session::feedback::Callbacks` are invoked on the polling thread; avoid blocking in callbacks to keep the state machine responsive.

## Diagnostics & Logging
- Core logging uses `SessionLogger` and helper macros in `iso15118/detail/helper.hpp`.
- EXI payload logging is available via `session::logging::set_session_log_callback` and `SSLConfig::enable_tls_key_logging`.
- SDP and connection setup paths log IPv6 addresses and connection state transitions.

## Tuning Knobs
- Disable warnings: `-DISO15118_COMPILE_OPTIONS_WARNING=""`.
- Toggle tests/coverage: `-DBUILD_TESTING=ON`, `ninja -C build iso15118_gcovr_coverage`.
- Disable EDM usage: `-DDISABLE_EDM=ON` to force FetchContent dependencies.
- Auto-download ISO 15118-20 schemas: `-DOPT_AUTODOWNLOAD_ISO20_SCHEMAS=ON` (requires `wget`).

## Troubleshooting & Performance Notes
- IPv6 interface lookup is required for sockets and SDP; verify the interface name exists and has an IPv6 address.
- TLS failures typically stem from certificate layout or missing root chains; confirm `SSLConfig` paths match the chosen backend.
- SDP relies on IPv6 multicast (`ff02::1`); ensure multicast is enabled and not blocked by firewall rules.
- The polling loop is time-sliced (default ~50 ms); tight latency requirements may need tighter polling and non-blocking callbacks.
- Useful log cues: "Using ethernet interface", "Start TLS server", "Incoming connection", "Handshake complete", and "Sequence Timeout 40secs is reached" indicate connection/session progress.
- On failures, check for "Failed to get ipv6 socket address", "Verify V2G root not found", or "A TCP/TLS connection could not be established" to pinpoint setup vs. TLS vs. transport issues.
- Debug checklist (grep in logs):
  - `grep -E "Using ethernet interface|Start TLS server|Incoming connection|Handshake complete" <logfile>`
  - `grep -E "Failed to get ipv6 socket address|Verify V2G root not found|A TCP/TLS connection could not be established" <logfile>`

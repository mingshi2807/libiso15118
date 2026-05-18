#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Vedecom 2026 : Contributors to EVerest

set -euo pipefail

readonly script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly repo_root="$(cd -- "${script_dir}/.." && pwd)"
readonly raw_build_dir="${BUILD_DIR:-build-pin-der}"
if [[ "${raw_build_dir}" = /* ]]; then
  readonly build_dir="${raw_build_dir}"
else
  readonly build_dir="${repo_root}/${raw_build_dir}"
fi
readonly ctest_parallel="${CTEST_PARALLEL_LEVEL:-$(nproc 2>/dev/null || echo 2)}"

readonly build_targets=(
  test_ac_der_control
  test_ac_der_secc_application_adapter
  test_service_discovery
  test_service_detail
  test_service_selection
  test_ac_charge_parameter_discovery
  test_ac_charge_loop
  test_fsm_ac_der_iec_flow
  test_exi_ac_charge_parameter_discovery
  test_exi_ac_charge_loop
  test_exi_service_detail
  test_exi_service_selection
  example_ac_der_secc_provider
  example_ac_der_secc_application_adapter
)

readonly focused_tests=(
  "test/iso15118/d20/test_ac_der_control"
  "test/iso15118/d20/test_ac_der_secc_application_adapter"
  "test/iso15118/states/test_service_discovery"
  "test/iso15118/states/test_service_detail"
  "test/iso15118/states/test_service_selection"
  "test/iso15118/states/test_ac_charge_parameter_discovery"
  "test/iso15118/states/test_ac_charge_loop"
  "test/iso15118/fsm/test_fsm_ac_der_iec_flow"
  "test/exi/cb/iso20/test_exi_ac_charge_parameter_discovery"
  "test/exi/cb/iso20/test_exi_ac_charge_loop"
  "test/exi/cb/iso20/test_exi_service_detail"
  "test/exi/cb/iso20/test_exi_service_selection"
)

readonly examples=(
  "examples/example_ac_der_secc_provider"
  "examples/example_ac_der_secc_application_adapter"
)

log_step() {
  printf '\n[ac-der-readiness] %s\n' "$*"
}

run_from_build_dir() {
  local relative_path="$1"
  local executable="${build_dir}/${relative_path}"

  if [[ ! -x "${executable}" ]]; then
    printf 'Missing executable: %s\n' "${executable}" >&2
    return 1
  fi

  "${executable}"
}

configure_if_needed() {
  if [[ -f "${build_dir}/CMakeCache.txt" ]]; then
    return
  fi

  log_step "Configure ${build_dir}"
  cmake -S "${repo_root}" -B "${build_dir}" -DBUILD_TESTING=ON -DISO15118_BUILD_EXAMPLES=ON "$@"
}

clean_stale_coverage_counters() {
  if [[ "${SKIP_GCDA_CLEAN:-0}" == "1" || ! -d "${build_dir}" ]]; then
    return
  fi

  log_step "Clean stale coverage counters"
  find "${build_dir}" -name '*.gcda' -delete
}

main() {
  log_step "AC_DER_IEC readiness gate"
  printf 'repo_root=%s\n' "${repo_root}"
  printf 'build_dir=%s\n' "${build_dir}"

  configure_if_needed "$@"
  clean_stale_coverage_counters

  log_step "Build focused tests and demos"
  cmake --build "${build_dir}" --target "${build_targets[@]}"

  log_step "Run focused AC_DER_IEC tests"
  for test_path in "${focused_tests[@]}"; do
    run_from_build_dir "${test_path}"
  done

  log_step "Run AC_DER_IEC demo examples"
  for example_path in "${examples[@]}"; do
    run_from_build_dir "${example_path}"
  done

  log_step "Run full CTest suite"
  ctest --test-dir "${build_dir}" --parallel "${ctest_parallel}" --output-on-failure

  log_step "Readiness gate passed"
}

main "$@"

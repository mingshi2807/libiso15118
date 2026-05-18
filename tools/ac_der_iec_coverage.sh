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

readonly output_dir="${AC_DER_COVERAGE_DIR:-${build_dir}/ac_der_coverage}"
readonly min_line_coverage="${AC_DER_COVERAGE_MIN_LINE:-70}"

readonly coverage_filters=(
  "${repo_root}/src/iso15118/d20/ac_der_control.cpp"
  "${repo_root}/src/iso15118/d20/state/ac_charge_parameter_discovery.cpp"
  "${repo_root}/src/iso15118/d20/state/ac_charge_loop.cpp"
  "${repo_root}/src/iso15118/d20/state/service_discovery.cpp"
  "${repo_root}/src/iso15118/d20/state/service_detail.cpp"
  "${repo_root}/src/iso15118/d20/state/service_selection.cpp"
  "${repo_root}/src/iso15118/message/ac_charge_parameter_discovery.cpp"
  "${repo_root}/src/iso15118/message/ac_charge_loop.cpp"
  "${repo_root}/src/iso15118/message/service_detail.cpp"
  "${repo_root}/src/iso15118/message/service_selection.cpp"
)

log_step() {
  printf '\n[ac-der-coverage] %s\n' "$*"
}

require_tool() {
  local tool="$1"

  if ! command -v "${tool}" >/dev/null 2>&1; then
    printf 'Missing required tool: %s\n' "${tool}" >&2
    return 1
  fi
}

detect_compiler() {
  if [[ ! -f "${build_dir}/CMakeCache.txt" ]]; then
    return
  fi

  sed -n 's/^CMAKE_CXX_COMPILER:FILEPATH=//p' "${build_dir}/CMakeCache.txt" | head -n 1
}

gcov_executable() {
  if [[ -n "${GCOV_EXECUTABLE:-}" ]]; then
    printf '%s\n' "${GCOV_EXECUTABLE}"
    return
  fi

  local compiler
  compiler="$(detect_compiler)"
  if [[ "${compiler}" == *clang++ || "${compiler}" == *clang ]]; then
    require_tool llvm-cov
    printf 'llvm-cov gcov\n'
    return
  fi

  require_tool gcov
  printf 'gcov\n'
}

run_readiness_gate() {
  log_step "Run AC_DER_IEC readiness gate to produce fresh counters"
  BUILD_DIR="${build_dir}" "${repo_root}/tools/ac_der_iec_readiness.sh"
}

generate_report() {
  local gcov_cmd="$1"
  local text_report="${output_dir}/ac_der_iec_coverage.txt"
  local xml_report="${output_dir}/ac_der_iec_coverage.xml"
  local json_report="${output_dir}/ac_der_iec_coverage_summary.json"
  local html_report="${output_dir}/html/index.html"
  local html_details_report="${output_dir}/html-details/index.html"

  mkdir -p "${output_dir}/html"

  local filter_args=()
  for filter in "${coverage_filters[@]}"; do
    filter_args+=(--filter "${filter}")
  done

  log_step "Generate focused AC_DER_IEC coverage report"
  printf 'repo_root=%s\n' "${repo_root}"
  printf 'build_dir=%s\n' "${build_dir}"
  printf 'gcov_executable=%s\n' "${gcov_cmd}"
  printf 'minimum_line_coverage=%s%%\n' "${min_line_coverage}"

  gcovr \
    --root "${repo_root}" \
    --object-directory "${build_dir}" \
    --gcov-executable "${gcov_cmd}" \
    "${filter_args[@]}" \
    --txt "${text_report}" \
    --xml "${xml_report}" \
    --json-summary-pretty \
    --json-summary "${json_report}" \
    --html "${html_report}" \
    --print-summary \
    --fail-under-line "${min_line_coverage}"

  if ! grep -Eq '^TOTAL[[:space:]]+[1-9][0-9]*[[:space:]]+' "${text_report}"; then
    printf 'Coverage report is empty: %s\n' "${text_report}" >&2
    return 1
  fi

  if [[ "${AC_DER_COVERAGE_HTML_DETAILS:-0}" == "1" ]]; then
    mkdir -p "${output_dir}/html-details"
    if ! gcovr \
      --root "${repo_root}" \
      --object-directory "${build_dir}" \
      --gcov-executable "${gcov_cmd}" \
      "${filter_args[@]}" \
      --html-details "${html_details_report}"; then
      printf 'Detailed HTML coverage is unavailable with the installed gcovr/Jinja2 stack.\n' >&2
      printf 'Summary HTML remains available at: %s\n' "${html_report}" >&2
    fi
  fi

  log_step "Focused coverage evidence generated"
  printf 'text=%s\n' "${text_report}"
  printf 'xml=%s\n' "${xml_report}"
  printf 'json=%s\n' "${json_report}"
  printf 'html=%s\n' "${html_report}"
  if [[ "${AC_DER_COVERAGE_HTML_DETAILS:-0}" == "1" && -f "${html_details_report}" ]]; then
    printf 'html_details=%s\n' "${html_details_report}"
  fi
}

main() {
  require_tool gcovr
  run_readiness_gate
  generate_report "$(gcov_executable)"
}

main "$@"

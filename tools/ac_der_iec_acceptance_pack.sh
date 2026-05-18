#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Vedecom 2026 : Contributors to EVerest

set -euo pipefail

readonly script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly repo_root="$(cd -- "${script_dir}/.." && pwd)"

usage() {
  cat <<'USAGE'
Usage: tools/ac_der_iec_acceptance_pack.sh [--run-evidence]

Print the AC_DER_IEC expert-review acceptance pack location and evidence checklist.
Use --run-evidence to run the readiness and focused coverage gates before review handoff.
USAGE
}

print_pack() {
  cat <<PACK
[ac-der-acceptance] Expert-review acceptance pack
repo_root=${repo_root}

Review pack:
  docs/ac_der_iec_acceptance_pack.md

Supporting evidence:
  docs/ac_der_iec_traceability.md
  docs/ac_der_secc_provider.md
  tools/ac_der_iec_readiness.sh
  tools/ac_der_iec_coverage.sh

Generated coverage artifacts after running tools/ac_der_iec_coverage.sh:
  build-pin-der/ac_der_coverage/ac_der_iec_coverage.txt
  build-pin-der/ac_der_coverage/ac_der_iec_coverage.xml
  build-pin-der/ac_der_coverage/ac_der_iec_coverage_summary.json
  build-pin-der/ac_der_coverage/html/index.html
PACK
}

run_evidence() {
  "${repo_root}/tools/ac_der_iec_readiness.sh"
  "${repo_root}/tools/ac_der_iec_coverage.sh"
}

main() {
  case "${1:-}" in
    "")
      print_pack
      ;;
    --run-evidence)
      print_pack
      run_evidence
      ;;
    -h|--help)
      usage
      ;;
    *)
      usage >&2
      return 2
      ;;
  esac
}

main "$@"

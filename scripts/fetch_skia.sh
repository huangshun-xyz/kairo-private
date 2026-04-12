#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

SKIA_DIR="${SKIA_DIR:-${ROOT_DIR}/src/third_party/skia}"
SKIA_GIT_URL="${SKIA_GIT_URL:-https://skia.googlesource.com/skia.git}"
SKIA_GIT_REF="${SKIA_GIT_REF:-main}"
SKIA_SYNC_DEPS="${SKIA_SYNC_DEPS:-0}"
SKIA_UPDATE="${SKIA_UPDATE:-0}"

mkdir -p "$(dirname -- "${SKIA_DIR}")"

if [[ ! -d "${SKIA_DIR}/.git" ]]; then
  git clone --depth 1 --branch "${SKIA_GIT_REF}" "${SKIA_GIT_URL}" "${SKIA_DIR}"
elif [[ "${SKIA_UPDATE}" == "1" ]]; then
  git -C "${SKIA_DIR}" fetch --depth 1 origin "${SKIA_GIT_REF}"
  git -C "${SKIA_DIR}" checkout --detach FETCH_HEAD
fi

if [[ "${SKIA_SYNC_DEPS}" == "1" ]]; then
  pushd "${SKIA_DIR}" >/dev/null
  python3 tools/git-sync-deps
  popd >/dev/null
fi

printf 'Skia checkout is ready at %s\n' "${SKIA_DIR}"

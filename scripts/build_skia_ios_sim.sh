#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

SKIA_DIR="${SKIA_DIR:-${ROOT_DIR}/src/third_party/skia}"
SKIA_OUT_DIR="${SKIA_OUT_DIR:-${SKIA_DIR}/out/ios_sim}"
IOS_SIM_ARCH="${IOS_SIM_ARCH:-$(uname -m)}"
IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-16.0}"

if [[ ! -d "${SKIA_DIR}/include" ]]; then
  printf 'Skia source tree was not found at %s\n' "${SKIA_DIR}" >&2
  printf 'Run scripts/fetch_skia.sh first.\n' >&2
  exit 1
fi

GN_BIN="${GN_BIN:-${SKIA_DIR}/bin/gn}"
NINJA_BIN="${NINJA_BIN:-${SKIA_DIR}/third_party/ninja/ninja}"

case "${IOS_SIM_ARCH}" in
  arm64)
    GN_TARGET_CPU="arm64"
    ;;
  x86_64|x64)
    IOS_SIM_ARCH="x86_64"
    GN_TARGET_CPU="x64"
    ;;
  *)
    printf 'Unsupported iOS simulator architecture: %s\n' "${IOS_SIM_ARCH}" >&2
    exit 1
    ;;
esac

if [[ ! -x "${GN_BIN}" ]]; then
  pushd "${SKIA_DIR}" >/dev/null
  python3 bin/fetch-gn
  popd >/dev/null
fi

if [[ ! -x "${NINJA_BIN}" ]]; then
  pushd "${SKIA_DIR}" >/dev/null
  python3 bin/fetch-ninja
  popd >/dev/null
fi

GN_ARGS="$(cat <<EOF
is_debug=true
is_official_build=false
target_os="ios"
target_cpu="${GN_TARGET_CPU}"
ios_use_simulator=true
ios_min_target="${IOS_DEPLOYMENT_TARGET}"
skia_enable_android_utils=false
skia_enable_fontmgr_empty=true
skia_enable_ganesh=true
skia_enable_graphite=false
skia_enable_pdf=false
skia_enable_skottie=false
skia_enable_skshaper=false
skia_enable_svg=false
skia_use_expat=false
skia_use_fontconfig=false
skia_use_fonthost_mac=false
skia_use_freetype=false
skia_use_gl=false
skia_use_harfbuzz=false
skia_use_icu=false
skia_use_jpeg_gainmaps=false
skia_use_libjpeg_turbo_decode=false
skia_use_libjpeg_turbo_encode=false
skia_use_libpng_decode=false
skia_use_libpng_encode=false
skia_use_libwebp_decode=false
skia_use_libwebp_encode=false
skia_use_metal=true
skia_use_piex=false
skia_use_vulkan=false
skia_use_wuffs=false
skia_use_zlib=false
EOF
)"

pushd "${SKIA_DIR}" >/dev/null
"${GN_BIN}" gen "${SKIA_OUT_DIR}" --args="${GN_ARGS}"
"${NINJA_BIN}" -C "${SKIA_OUT_DIR}" skia
popd >/dev/null

printf 'Built Skia for iOS Simulator (%s) at %s\n' "${IOS_SIM_ARCH}" "${SKIA_OUT_DIR}"

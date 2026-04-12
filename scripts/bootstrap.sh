#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/ios_sim}"
IOS_SIM_ARCH="${IOS_SIM_ARCH:-$(uname -m)}"
IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-16.0}"
CONFIGURATION="${CONFIGURATION:-Debug}"
CMAKE_GENERATOR="${CMAKE_GENERATOR:-Xcode}"
DERIVED_DATA_DIR="${DERIVED_DATA_DIR:-${BUILD_DIR}/DerivedData}"
IOS_DESTINATION="${IOS_DESTINATION:-}"

if [[ "${CMAKE_GENERATOR}" != "Xcode" ]]; then
  printf 'bootstrap.sh only supports the Xcode generator for the iOS demo.\n' >&2
  exit 1
fi

case "${IOS_SIM_ARCH}" in
  arm64)
    CMAKE_IOS_ARCH="arm64"
    ;;
  x86_64|x64)
    CMAKE_IOS_ARCH="x86_64"
    IOS_SIM_ARCH="x86_64"
    ;;
  *)
    printf 'Unsupported iOS simulator architecture: %s\n' "${IOS_SIM_ARCH}" >&2
    exit 1
    ;;
esac

if [[ -z "${IOS_DESTINATION}" ]]; then
  IOS_RUNTIME=""
  IOS_DEVICE_NAME=""
  while IFS= read -r line; do
    line="$(printf '%s' "${line}" | sed 's/[[:space:]]*$//')"

    if [[ "${line}" =~ ^--\ iOS\ (.+)\ --$ ]]; then
      IOS_RUNTIME="${BASH_REMATCH[1]}"
      continue
    fi

    if [[ -n "${IOS_RUNTIME}" && "${line}" =~ ^[[:space:]]+(iPhone.+)\ \([0-9A-F-]+\)\ \((Shutdown|Booted)\)$ ]]; then
      IOS_DEVICE_NAME="${BASH_REMATCH[1]}"
      break
    fi
  done < <(xcrun simctl list devices available 2>/dev/null || true)

  if [[ -n "${IOS_RUNTIME}" && -n "${IOS_DEVICE_NAME}" ]]; then
    IOS_DESTINATION="platform=iOS Simulator,name=${IOS_DEVICE_NAME},OS=${IOS_RUNTIME}"
  else
    IOS_DESTINATION="generic/platform=iOS Simulator"
  fi
fi

"${SCRIPT_DIR}/fetch_skia.sh"
IOS_SIM_ARCH="${IOS_SIM_ARCH}" IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}" "${SCRIPT_DIR}/build_skia_ios_sim.sh"

cmake \
  --fresh \
  -S "${ROOT_DIR}/app/ios" \
  -B "${BUILD_DIR}" \
  -G "${CMAKE_GENERATOR}" \
  -U KAIRO_SKIA_DIR \
  -U KAIRO_SKIA_OUT_DIR \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_ARCHITECTURES="${CMAKE_IOS_ARCH}" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}"

xcodebuild \
  -project "${BUILD_DIR}/kairo_ios_demo.xcodeproj" \
  -scheme kairo_ios_demo \
  -configuration "${CONFIGURATION}" \
  -destination "${IOS_DESTINATION}" \
  -derivedDataPath "${DERIVED_DATA_DIR}" \
  build

printf 'Generated Xcode project: %s\n' "${BUILD_DIR}/kairo_ios_demo.xcodeproj"
printf 'Built app bundle root: %s\n' "${BUILD_DIR}\n"

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
IOS_DESTINATION="${IOS_DESTINATION:-generic/platform=iOS Simulator}"

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

"${SCRIPT_DIR}/fetch_skia.sh"
IOS_SIM_ARCH="${IOS_SIM_ARCH}" IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}" "${SCRIPT_DIR}/build_skia_ios_sim.sh"

cmake \
  -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -G "${CMAKE_GENERATOR}" \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_ARCHITECTURES="${CMAKE_IOS_ARCH}" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}" \
  -DKAIRO_ENABLE_IOS_DEMO=ON

xcodebuild \
  -project "${BUILD_DIR}/kairo.xcodeproj" \
  -scheme kairo_ios_demo \
  -configuration "${CONFIGURATION}" \
  -destination "${IOS_DESTINATION}" \
  -derivedDataPath "${DERIVED_DATA_DIR}" \
  build

printf 'Generated Xcode project: %s\n' "${BUILD_DIR}/kairo.xcodeproj"
printf 'Built app bundle: %s\n' "${BUILD_DIR}/app/ios/${CONFIGURATION}-iphonesimulator/kairo_ios_demo.app"

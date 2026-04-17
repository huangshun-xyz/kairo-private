cmake_minimum_required(VERSION 3.29)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0")
  endif()

  if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
    set(KAIRO_SWIFT_PLATFORM_SUFFIX "-simulator")
  else()
    set(KAIRO_SWIFT_PLATFORM_SUFFIX "")
  endif()

  if(NOT CMAKE_Swift_COMPILER_TARGET)
    if(CMAKE_OSX_ARCHITECTURES)
      list(GET CMAKE_OSX_ARCHITECTURES 0 KAIRO_SWIFT_ARCH)
    else()
      set(KAIRO_SWIFT_ARCH "arm64")
    endif()
    set(
      CMAKE_Swift_COMPILER_TARGET
      "${KAIRO_SWIFT_ARCH}-apple-ios${CMAKE_OSX_DEPLOYMENT_TARGET}${KAIRO_SWIFT_PLATFORM_SUFFIX}"
    )
  endif()

  set(CMAKE_Swift_FLAGS_INIT "-module-cache-path /tmp/kairo-swift-module-cache")
  set(CMAKE_Swift_COMPILER_FORCED TRUE)
  set(CMAKE_Swift_COMPILER_WORKS TRUE)
endif()

project(kairo_ios_demo LANGUAGES C CXX OBJC OBJCXX Swift)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJC_STANDARD 11)
set(CMAKE_OBJC_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
set(CMAKE_Swift_LANGUAGE_VERSION 5)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  message(FATAL_ERROR "kairo_ios_demo must be configured with CMAKE_SYSTEM_NAME=iOS.")
endif()

get_filename_component(KAIRO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

include("${KAIRO_ROOT_DIR}/src/cmake/skia.cmake")
kairo_configure_skia()

add_subdirectory("${KAIRO_ROOT_DIR}/src" "${CMAKE_BINARY_DIR}/kairo_sdk")

find_library(UIKIT_FRAMEWORK UIKit REQUIRED)
find_library(COREGRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)

set(KAIRO_IOS_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}")
if(NOT KAIRO_IOS_DEPLOYMENT_TARGET)
  set(KAIRO_IOS_DEPLOYMENT_TARGET "16.0")
endif()

add_executable(
  kairo_ios_demo
  MACOSX_BUNDLE
  AppDelegate.mm
  ViewController.mm
  MainView.swift
  main.mm
  "${KAIRO_ROOT_DIR}/platform/ios/KRChartView.swift"
)

target_include_directories(
  kairo_ios_demo
  PRIVATE
    "${KAIRO_ROOT_DIR}/src/base"
    "${KAIRO_ROOT_DIR}/src/runtime"
    "${KAIRO_ROOT_DIR}/src/platform/ios"
    "${KAIRO_ROOT_DIR}/src/public"
    "${KAIRO_ROOT_DIR}/platform/ios"
)

target_link_libraries(
  kairo_ios_demo
  PRIVATE
    kairo_platform_ios
    c++
    "${UIKIT_FRAMEWORK}"
    "${COREGRAPHICS_FRAMEWORK}"
    "${QUARTZCORE_FRAMEWORK}"
)

target_compile_options(
  kairo_ios_demo
  PRIVATE
    "$<$<COMPILE_LANGUAGE:Swift>:-import-objc-header>"
    "$<$<COMPILE_LANGUAGE:Swift>:${KAIRO_ROOT_DIR}/app/ios/kairo_ios_demo-Bridging-Header.h>"
)

set_target_properties(
  kairo_ios_demo
  PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.kairo.demo"
    MACOSX_BUNDLE_BUNDLE_NAME "kairo_ios_demo"
    MACOSX_BUNDLE_EXECUTABLE_NAME "kairo_ios_demo"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    MACOSX_BUNDLE_BUNDLE_VERSION "1"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.kairo.demo"
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${KAIRO_ROOT_DIR}/app/ios/kairo_ios_demo-Bridging-Header.h"
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${KAIRO_IOS_DEPLOYMENT_TARGET}"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
    XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphonesimulator"
)

set_property(
  TARGET kairo_ios_demo
  PROPERTY SWIFT_OBJC_BRIDGING_HEADER
    "${KAIRO_ROOT_DIR}/app/ios/kairo_ios_demo-Bridging-Header.h"
)

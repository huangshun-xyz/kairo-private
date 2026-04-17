cmake_minimum_required(VERSION 3.29)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  project(kairo_sdk LANGUAGES C CXX OBJC OBJCXX)
else()
  project(kairo_sdk LANGUAGES C CXX)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/skia.cmake")
kairo_configure_skia()

add_library(kairo_base INTERFACE)
add_library(kairo::base ALIAS kairo_base)

target_include_directories(
  kairo_base
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/base"
)

add_library(
  kairo_core
  STATIC
    core/chart.cc
    core/chart_controller.cc
    core/linear_y_scale.cc
    core/pane.cc
    core/scroll_interaction.cc
    core/uniform_bar_x_scale.cc
)
add_library(kairo::core ALIAS kairo_core)

target_include_directories(
  kairo_core
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/core"
)

target_link_libraries(
  kairo_core
  PUBLIC
    kairo_skia
)

add_library(
  kairo_render
  STATIC
    render/candle_series.cc
    render/crosshair_layer.cc
    render/crosshair_overlay.cc
    render/demo_chart_factory.cc
    render/grid_layer.cc
    render/volume_series.cc
)
add_library(kairo::render ALIAS kairo_render)

target_include_directories(
  kairo_render
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/render"
    "${CMAKE_CURRENT_SOURCE_DIR}/core"
)

target_link_libraries(
  kairo_render
  PUBLIC
    kairo_core
    kairo_skia
)

add_library(
  kairo_public
  STATIC
    public/kchart.cc
)
add_library(kairo::public ALIAS kairo_public)

target_include_directories(
  kairo_public
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/public"
    "${CMAKE_CURRENT_SOURCE_DIR}/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/render"
)

target_link_libraries(
  kairo_public
  PUBLIC
    kairo_base
    kairo_render
    kairo_core
    kairo_skia
)

add_library(
  kairo_runtime
  STATIC
    runtime/chart_runtime.cc
    runtime/scroll_animator.cc
)
add_library(kairo::runtime ALIAS kairo_runtime)

target_include_directories(
  kairo_runtime
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime"
    "${CMAKE_CURRENT_SOURCE_DIR}/public"
    "${CMAKE_CURRENT_SOURCE_DIR}/base"
)

target_link_libraries(
  kairo_runtime
  PUBLIC
    kairo_public
    kairo_base
)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
  find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)

  add_library(
    kairo_platform_ios
    STATIC
      platform/ios/KRIOSChartHost.mm
      platform/ios/display_link_frame_source.mm
      platform/ios/main_run_loop_task_runner.mm
      platform/ios/system_tick_clock.mm
  )
  add_library(kairo::platform_ios ALIAS kairo_platform_ios)

  target_include_directories(
    kairo_platform_ios
    PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}/platform/ios"
      "${CMAKE_CURRENT_SOURCE_DIR}/base"
      "${CMAKE_CURRENT_SOURCE_DIR}/runtime"
  )

  target_link_libraries(
    kairo_platform_ios
    PUBLIC
      kairo_runtime
      kairo_base
      "${FOUNDATION_FRAMEWORK}"
      "${QUARTZCORE_FRAMEWORK}"
  )
endif()

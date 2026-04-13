cmake_minimum_required(VERSION 3.29)

project(kairo_sdk LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/skia.cmake")
kairo_configure_skia()

add_library(
  kairo_core
  STATIC
    core/chart.cc
    core/chart_controller.cc
    core/linear_y_scale.cc
    core/pane.cc
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

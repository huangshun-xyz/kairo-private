add_library(kairo_render STATIC skia_renderer.cc)
add_library(kairo::render ALIAS kairo_render)

target_include_directories(
  kairo_render
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/core"
)

target_link_libraries(
  kairo_render
  PUBLIC
    kairo_core
    kairo_skia
)

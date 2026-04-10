add_library(kairo_core STATIC chart.cc)
add_library(kairo::core ALIAS kairo_core)

target_include_directories(
  kairo_core
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

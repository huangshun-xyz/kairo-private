cmake_minimum_required(VERSION 3.29)

project(kairo LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJC_STANDARD 11)
set(CMAKE_OBJC_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)

option(KAIRO_ENABLE_IOS_DEMO "Build the minimal iOS demo target" ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/skia.cmake")
kairo_configure_skia()

add_subdirectory(core)
add_subdirectory(render)

if(KAIRO_ENABLE_IOS_DEMO)
  add_subdirectory(app/ios)
endif()

function(kairo_configure_skia)
  if(TARGET kairo_skia)
    return()
  endif()

  get_filename_component(KAIRO_SDK_ROOT "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)

  set(
    KAIRO_SKIA_DIR
    "${KAIRO_SDK_ROOT}/third_party/skia"
    CACHE PATH
    "Location of the checked out Skia source tree"
  )
  set(
    KAIRO_SKIA_OUT_DIR
    "${KAIRO_SKIA_DIR}/out/ios_sim"
    CACHE PATH
    "Location of the Skia iOS Simulator build artifacts"
  )

  if(NOT EXISTS "${KAIRO_SKIA_DIR}/include/core/SkCanvas.h")
    message(
      FATAL_ERROR
      "Skia headers were not found under ${KAIRO_SKIA_DIR}. Run scripts/fetch_skia.sh first."
    )
  endif()

  if(NOT EXISTS "${KAIRO_SKIA_OUT_DIR}/libskia.a")
    message(
      FATAL_ERROR
      "Skia iOS Simulator library was not found under ${KAIRO_SKIA_OUT_DIR}. "
      "Run scripts/build_skia_ios_sim.sh first."
    )
  endif()

  add_library(kairo_skia INTERFACE)
  add_library(kairo::skia ALIAS kairo_skia)

  target_include_directories(
    kairo_skia
    INTERFACE
      "${KAIRO_SKIA_DIR}"
      "${KAIRO_SKIA_DIR}/modules"
  )

  target_link_libraries(
    kairo_skia
    INTERFACE
      "${KAIRO_SKIA_OUT_DIR}/libskia.a"
  )

  if(APPLE)
    foreach(framework CoreFoundation CoreGraphics CoreText Foundation Metal QuartzCore UIKit)
      set(framework_var "KAIRO_${framework}_FRAMEWORK")
      find_library(${framework_var} NAMES ${framework})
      if(${framework_var})
        target_link_libraries(kairo_skia INTERFACE "${${framework_var}}")
      endif()
    endforeach()
  endif()
endfunction()

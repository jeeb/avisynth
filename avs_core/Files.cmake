FILE(GLOB AvsCore_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*.c"
  "*.cpp"
  "*.h"

  "include/*.h"
  "include/avs/*.h"

  "convert/*.c"
  "convert/*.cpp"
  "convert/*.h"

  "core/*.c"
  "core/*.cpp"
  "core/*.h"

  "core/parser/*.c"
  "core/parser/*.cpp"
  "core/parser/*.h"

  "core/fonts/*.cpp"
  "core/fonts/*.h"

  "filters/*.c"
  "filters/*.cpp"
  "filters/*.h"

  "filters/conditional/*.c"
  "filters/conditional/*.cpp"
  "filters/conditional/*.h"

  "filters/overlay/*.c"
  "filters/overlay/*.cpp"
  "filters/overlay/*.h"

  "filters/exprfilter/*.cpp"
  "filters/exprfilter/*.h"

)

IF(ENABLE_SIMD)
  FILE(GLOB Conditional_Filter_Cpu_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "filters/conditional/intel/*.cpp"
    "filters/conditional/intel/*.h")
  LIST(REMOVE_ITEM AvsCore_Sources "filters/conditional/conditional_functions.cpp" "filters/conditional/conditional_functions.h")
  LIST(APPEND AvsCore_Sources "${Conditional_Filter_Cpu_Sources}")

  FILE(GLOB Convert_Cpu_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "convert/intel/*.cpp"
    "convert/intel/*.h")
  LIST(REMOVE_ITEM AvsCore_Sources "convert/convert_planar.cpp"
                                   "convert/convert_rgb.cpp"
                                   "convert/convert.cpp"
                                   "convert/convert.h"
                                   "convert/convert_yuy2.cpp"
                                   "convert/convert_yv12.cpp"
                                   "convert/convert_yv12.h")
  LIST(APPEND AvsCore_Sources "${Convert_Cpu_Sources}")

  FILE(GLOB Filters_Cpu_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "filters/intel/*.cpp"
    "filters/intel/*.h")
  LIST(REMOVE_ITEM AvsCore_Sources "filters/focus.cpp"
                                   "filters/focus.h"
                                   "filters/greyscale.cpp"
                                   "filters/greyscale.h"
                                   "filters/layer.cpp"
                                   "filters/layer.h"
                                   "filters/levels.cpp"
                                   "filters/levels.h"
                                   "filters/limiter.cpp"
                                   "filters/limiter.h"
                                   "filters/merge.cpp"
                                   "filters/merge.h"
                                   "filters/planeswap.cpp"
                                   "filters/planeswap.h"
                                   "filters/resample.cpp"
                                   "filters/resample.h"
                                   "filters/resize.cpp"
                                   "filters/resize.h"
                                   "filters/text-overlay.cpp"
                                   "filters/text-overlay.h"
                                   "filters/turn.cpp"
                                   "filters/turn.h")
  LIST(APPEND AvsCore_Sources "${Filters_Cpu_Sources}")
ENDIF()

IF( MSVC OR MINGW )
# AviSource is Windows-only, because it depends on Video for Windows
  FILE(GLOB AvsCore_Sources_AviSource RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "filters/AviSource/*.c"
    "filters/AviSource/*.cpp"
    "filters/AviSource/*.h")
  LIST(APPEND AvsCore_Sources "${AvsCore_Sources_AviSource}")
ELSE()
  LIST(APPEND AvsCore_Sources "core/parser/os/win32_string_compat.cpp")
  LIST(APPEND AvsCore_Sources "core/parser/os/win32_string_compat.h")
ENDIF()

IF( MSVC OR MINGW )
    # Export definitions in general are not needed on x64 and only cause warnings,
    # unfortunately we still must need a .def file for some COM functions.
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      LIST(APPEND AvsCore_Sources "core/avisynth64.def")
    else()
      LIST(APPEND AvsCore_Sources "core/avisynth.def")
    endif()
ENDIF()

IF( MSVC_IDE )
    # Ninja, unfortunately, seems to have some issues with using rc.exe
    LIST(APPEND AvsCore_Sources "core/avisynth.rc")
ENDIF()

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

  "core/os/PluginManager_win32.cpp"
  "core/os/PluginManager.h"

  "core/parser/*.c"
  "core/parser/*.cpp"
  "core/parser/*.h"

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

IF( MSVC OR MINGW )
# AviSource is Windows-only, because it depends on Video for Windows
  FILE(GLOB AvsCore_Sources_AviSource RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "filters/AviSource/*.c"
    "filters/AviSource/*.cpp"
    "filters/AviSource/*.h")
  LIST(APPEND AvsCore_Sources "${AvsCore_Sources_AviSource}")
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

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
  "core/avisynth.rc"

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

  "filters/AviSource/*.c"
  "filters/AviSource/*.cpp"
  "filters/AviSource/*.h"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  # Export definitions are not needed on x64 and only cause warnings,
  # so add them only when compiling for 32-bits.
  LIST(APPEND AvsCore_Sources "core/avisynth.def")
endif() 
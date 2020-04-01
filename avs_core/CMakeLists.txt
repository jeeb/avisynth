# We need CMake 3.8 at least, because we require
# CMAKE_CXX_STANDARD to be set to C++17.
CMAKE_MINIMUM_REQUIRED( VERSION 3.8.2 )

if (WIN32)
  set(CoreName "AviSynth")
else()
  set(CoreName "avisynth")
endif()

# Create library
project("AvsCore" VERSION "${PROJECT_VERSION}" LANGUAGES CXX)
Include("Files.cmake")
add_library("AvsCore" SHARED ${AvsCore_Sources})
set_target_properties("AvsCore" PROPERTIES "OUTPUT_NAME" "${CoreName}")
if (NOT WIN32)
  set_target_properties("AvsCore" PROPERTIES VERSION "${PROJECT_VERSION}")
endif()
if (MINGW)
  set_target_properties("AvsCore" PROPERTIES PREFIX "")
  set_target_properties("AvsCore" PROPERTIES IMPORT_PREFIX "")
endif()

# Automatically group source files according to directory structure
foreach(FILE ${AvsCore_Sources})
  get_filename_component(PARENT_DIR "${FILE}" PATH)

  string(REGEX REPLACE "(\\./)" "" GROUP "${PARENT_DIR}")
  string(REPLACE "/" "\\" GROUP "${GROUP}")

  # group into "Source Files" and "Header Files"
  if ("${FILE}" MATCHES ".*\\.cpp")
    set(GROUP "Source Files\\${GROUP}")
  elseif("${FILE}" MATCHES ".*\\.h")
    set(GROUP "Header Files\\${GROUP}")
  endif()

  source_group("${GROUP}" FILES "${FILE}")
endforeach()

if (MSVC_IDE)
  IF(CLANG_IN_VS STREQUAL "1")
      # special SSSE3 option for source files with *_ssse3.cpp pattern
      file(GLOB_RECURSE SRCS_SSSE3 "*_ssse3.cpp")
      set_source_files_properties(${SRCS_SSSE3} PROPERTIES COMPILE_FLAGS " -mssse3 ")

      # special SSE4.1 option for source files with *_sse41.cpp pattern
      file(GLOB_RECURSE SRCS_SSE41 "*_sse41.cpp")
      set_source_files_properties(${SRCS_SSE41} PROPERTIES COMPILE_FLAGS " -msse4.1 ")

      # special AVX option for source files with *_avx.cpp pattern
      file(GLOB_RECURSE SRCS_AVX "*_avx.cpp")
      set_source_files_properties(${SRCS_AVX} PROPERTIES COMPILE_FLAGS " -mavx ")

      # special AVX2 option for source files with *_avx2.cpp pattern
      file(GLOB_RECURSE SRCS_AVX2 "*_avx2.cpp")
      set_source_files_properties(${SRCS_AVX2} PROPERTIES COMPILE_FLAGS " -mavx2 -mfma ")
  ELSE()
      # special AVX option for source files with *_avx.cpp pattern
      file(GLOB_RECURSE SRCS_AVX "*_avx.cpp")
      set_source_files_properties(${SRCS_AVX} PROPERTIES COMPILE_FLAGS " /arch:AVX ")

      # special AVX2 option for source files with *_avx2.cpp pattern
      file(GLOB_RECURSE SRCS_AVX2 "*_avx2.cpp")
      set_source_files_properties(${SRCS_AVX2} PROPERTIES COMPILE_FLAGS " /arch:AVX2 ")
  ENDIF()
else()
  # special SSSE3 option for source files with *_ssse3.cpp pattern
  file(GLOB_RECURSE SRCS_SSSE3 "*_ssse3.cpp")
  set_source_files_properties(${SRCS_SSSE3} PROPERTIES COMPILE_FLAGS " -mssse3 ")

  # special SSE4.1 option for source files with *_sse41.cpp pattern
  file(GLOB_RECURSE SRCS_SSE41 "*_sse41.cpp")
  set_source_files_properties(${SRCS_SSE41} PROPERTIES COMPILE_FLAGS " -msse4.1 ")

  # special AVX option for source files with *_avx.cpp pattern
  file(GLOB_RECURSE SRCS_AVX "*_avx.cpp")
  set_source_files_properties(${SRCS_AVX} PROPERTIES COMPILE_FLAGS -mavx )

  # special AVX2 option for source files with *_avx2.cpp pattern
  file(GLOB_RECURSE SRCS_AVX2 "*_avx2.cpp")
  set_source_files_properties(${SRCS_AVX2} PROPERTIES COMPILE_FLAGS "-mavx2 -mfma")
endif()

# Specify include directories
target_include_directories("AvsCore" PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Specify preprocessor definitions
target_compile_definitions("AvsCore" PRIVATE BUILDING_AVSCORE)

# stdc++fs was mainlined into stdc++ in GCC 9, but GCC 8 can build it too
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    set(FSLIB "stdc++fs")
endif()

# Windows DLL dependencies
if (MSVC OR MINGW)
  target_link_libraries("AvsCore" "uuid" "winmm" "vfw32" "msacm32" "gdi32" "user32" "advapi32" "ole32" "imagehlp")
else()
  target_link_libraries("AvsCore" "pthread" "dl" "${FSLIB}")
endif()

if (MSVC_IDE)
  # Copy output to a common folder for easy deployment
  add_custom_command(
    TARGET AvsCore
    POST_BUILD
    COMMAND xcopy /Y \"$(TargetPath)\" \"${CMAKE_BINARY_DIR}/Output\"
  )
  IF(NOT CLANG_IN_VS STREQUAL "1")
    # LLVM does not generate exp or don't know how to do it
    add_custom_command(
      TARGET AvsCore
      POST_BUILD
      COMMAND xcopy /Y \"$(TargetDir)AviSynth.exp\" \"${CMAKE_BINARY_DIR}/Output/c_api\"
    )
  ENDIF()
  add_custom_command(
    TARGET AvsCore
    POST_BUILD
    COMMAND xcopy /Y \"$(TargetDir)AviSynth.lib\" \"${CMAKE_BINARY_DIR}/Output/c_api\"
  )
endif()

# Determine target architecture
include("${CMAKE_CURRENT_LIST_DIR}/TargetArch.cmake")
target_architecture(AVS_ARCH)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/core/arch.h.in ${CMAKE_CURRENT_BINARY_DIR}/arch.h @ONLY)

# Dynamically generate the sequential version info from Git
# Based on the example here: http://www.cmake.org/pipermail/cmake/2010-July/038015.html
FIND_PACKAGE(Git)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
ADD_CUSTOM_TARGET(
    VersionGen
    ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/core/version.h.in
                     -D DST=${CMAKE_CURRENT_BINARY_DIR}/version.h
                     -D GIT=${GIT_EXECUTABLE}
                     -D REPO=${CMAKE_SOURCE_DIR}
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/Version.cmake
)
ADD_DEPENDENCIES("AvsCore" VersionGen)

# Generate pkg-config file
get_target_property(LIB_NAME AvsCore OUTPUT_NAME)
set(LIBS "-l${LIB_NAME}")
CONFIGURE_FILE("avisynth.pc.in" "avisynth.pc" @ONLY)

# Generate avisynth.conf
CONFIGURE_FILE("avisynth_conf.h.in" "avisynth_conf.h" @ONLY)

INSTALL(TARGETS AvsCore
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")

INSTALL(DIRECTORY "include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/avisynth")

INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/avisynth.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

include_directories(${AvsCore_SOURCE_DIR}/include)

option(BUILD_IMAGESEQ "Build ImageSeq plugin" ON)
option(BUILD_DIRECTSHOWSOURCE "Build DirectShowSource plugin" OFF)
option(BUILD_TIMESTRETCH "Build TimeStretch plugin" ON)
option(BUILD_SHIBATCH "Build Shibatch plugin" ON)
option(BUILD_VDUBFILTER "Build VDubFilter plugin" ON)
option(BUILD_CONVERTSTACKED "Build ConvertStacked plugin" ON)
#option(BUILD_VFAPIFILTER "Build VFAPIFilter plugin" ON)

if (BUILD_IMAGESEQ)
add_subdirectory("ImageSeq")
endif()

if (MSVC OR MINGW)
  if(BUILD_DIRECTSHOWSOURCE)
    add_subdirectory("DirectShowSource")
  endif()
endif()

if (BUILD_TIMESTRETCH)
add_subdirectory("TimeStretch")
endif()

if (BUILD_SHIBATCH)
add_subdirectory("Shibatch")
endif()

if (BUILD_VDUBFILTER)
add_subdirectory("VDubFilter")
endif()

if (BUILD_CONVERTSTACKED)
add_subdirectory("ConvertStacked")
endif()

#if (BUILD_VFAPIFILTER)
#add_subdirectory("VFAPIFilter")
#endif()

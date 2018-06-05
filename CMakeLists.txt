# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

project("AviSynth+")

# Avoid uselessly linking to unused libraries
set(CMAKE_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)

option(ENABLE_PLUGINS "Build set of default external plugins" ON)

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo)
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset the configurations to what we need" FORCE)
endif()

IF( MSVC_IDE )  # Check for Visual Studio

  #1800      = VS 12.0 (v120 toolset)
  #1900      = VS 14.0 (v140 toolset)
  #1910-1919 = VS 15.0 (v141 toolset)

  IF( MSVC_VERSION VERSION_LESS 1910 )
    MESSAGE(FATAL_ERROR "Visual C++ 2017 or newer required.")
  ENDIF()

  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Output/plugins")
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Output/system")
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Output/c_api")

  IF(CMAKE_GENERATOR_TOOLSET STREQUAL "v141_clang_c2")
     #1900 is reported
    message("v141_clang_c2 toolset was specified via -T. Reported MSVC_VERSION is: ${MSVC_VERSION}")
    set(CLANG_IN_VS "1")
  ENDIF()
  
  # We want our project to also run on Windows XP
  # 1900 (VS2015) is not supported but we leave here
  IF(MSVC_VERSION VERSION_LESS 1910 )
    IF(NOT CLANG_IN_VS STREQUAL "1")
      set(CMAKE_GENERATOR_TOOLSET "v140_xp" CACHE STRING "The compiler toolset to use for Visual Studio." FORCE) # VS2015
      # https://connect.microsoft.com/VisualStudio/feedback/details/1789709/visual-c-2015-runtime-broken-on-windows-server-2003-c-11-magic-statics
      add_definitions("/Zc:threadSafeInit-")
  ENDIF()
  ELSE()
    IF(NOT CLANG_IN_VS STREQUAL "1")
      set(CMAKE_GENERATOR_TOOLSET "v141_xp" CACHE STRING "The compiler toolset to use for Visual Studio." FORCE) # VS2017
      # https://connect.microsoft.com/VisualStudio/feedback/details/1789709/visual-c-2015-runtime-broken-on-windows-server-2003-c-11-magic-statics
      add_definitions("/Zc:threadSafeInit-")
    ENDIF()
  ENDIF()

  IF(CLANG_IN_VS STREQUAL "1")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fexceptions")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-inconsistent-missing-override")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override")
  ELSE()
	  # Enable C++ with SEH exceptions
	  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHa")
	  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")
	  # or add_compile_options( /EHa ) for CMake>=3?
  ENDIF()
  # Prevent VC++ from complaining about not using MS-specific functions
  add_definitions("/D _CRT_SECURE_NO_WARNINGS /D _SECURE_SCL=0")
  
  # Enable CRT heap debugging - only effective in debug builds
  add_definitions("/D _CRTDBG_MAP_ALLOC")
  
  add_definitions("/D __SSE2__")

  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # VC++ enables the SSE2 instruction set by default even on 32-bits. Step back a bit.
    #add_definitions("/arch:SSE")
    add_definitions("/arch:SSE2") # Better use this one, it's 2017 now, and helps optimizing hbd stuff still in C
  endif() 

  # Set additional optimization flags
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Oy /Ot /GS- /Oi")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oy /Ot /GS- /Oi")

  # Set C++17 flag
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c++17")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
ELSE()

  # c++17: if constexpr() and others from gcc 7
  SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=gnu++17" ) # or -std=c++17 w/o gnu c++17 extensions
  SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -march=native" )

ENDIF()

add_subdirectory("avs_core")
if(ENABLE_PLUGINS)
  add_subdirectory("plugins")
endif()

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
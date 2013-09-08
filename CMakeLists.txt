# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

project("AvsMod")

#Avoid uselessly linking to unused libraries
set(CMAKE_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo)
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset the configurations to what we need" FORCE)
endif()

IF( MSVC_IDE )  # Check for Visual Studio
  IF( NOT MSVC_VERSION VERSION_LESS 1700 )  # Check for v11 (VS 2012)
    # We want our project to also run on Windows XP
    set(CMAKE_GENERATOR_TOOLSET "v110_xp" CACHE STRING "The compiler toolset to use for Visual Studio." FORCE)
  ENDIF()
ENDIF()

add_subdirectory("avs_core")
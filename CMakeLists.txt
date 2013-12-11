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

  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Output/plugins")
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Output/system")

  IF( NOT MSVC_VERSION VERSION_LESS 1700 )  # Check for v11 (VS 2012)
    # We want our project to also run on Windows XP
    set(CMAKE_GENERATOR_TOOLSET "v110_xp" CACHE STRING "The compiler toolset to use for Visual Studio." FORCE)
  ENDIF()

  # Enable C++ with SEH exceptions
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHa")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")

  # Prevent VC++ from complaining about not using MS-specific functions
  add_definitions("/D _CRT_SECURE_NO_WARNINGS /D _SECURE_SCL=0 /D _ITERATOR_DEBUG_LEVEL=0")
  
  # Set additional optimization flags
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Oy /Ot /GS-")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oy /Ot /GS-")

ENDIF()

add_subdirectory("avs_core")
add_subdirectory("plugins")

# Dynamically generate the version info so that the sequential revision in Version() doesn't literally say 'rXXX'
# Based on an example given on the CMake mailing list: http://www.cmake.org/pipermail/cmake/2010-July/038015.html

FIND_PACKAGE(Git)

# Protect against the (somewhat typical) case of git being installed to Program Files
set(GIT "${GIT_EXECUTABLE}")

FILE(WRITE ${CMAKE_BINARY_DIR}/main.c
    "\#include \"avs_core/core/version.h\"\nint main(void){return 0;}\n"
)

FILE(WRITE ${CMAKE_BINARY_DIR}/version.h.in
    "\#define AVS_SEQREV \"@AVS_SEQREV@\"\n"
)

FILE(WRITE ${CMAKE_BINARY_DIR}/version.cmake
    "EXECUTE_PROCESS(
         COMMAND \"@GIT@\" rev-list --count HEAD
         OUTPUT_VARIABLE AVS_SEQREV
         OUTPUT_STRIP_TRAILING_WHITESPACE
     )
     CONFIGURE_FILE(\${SRC} \${DST} @ONLY)
")

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_EXECUTABLE(main main.c)
ADD_CUSTOM_TARGET(
    version
    ${CMAKE_COMMAND} -D SRC=${CMAKE_BINARY_DIR}/version.h.in
                     -D DST=${CMAKE_BINARY_DIR}/avs_core/core/version.h
                     -P ${CMAKE_BINARY_DIR}/version.cmake
)
ADD_DEPENDENCIES(main version)

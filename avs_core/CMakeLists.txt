# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

# Create library
project("AvsCore")
Include("Files.cmake")
add_library("AvsCore" SHARED ${AvsCore_Sources})
set_target_properties("AvsCore" PROPERTIES "OUTPUT_NAME" "AviSynth")

# Specify include directories
target_include_directories("AvsCore" PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Specify preprocessor definitions
target_compile_definitions("AvsCore" PRIVATE BUILDING_AVSCORE)

# Windows DLL dependencies 
target_link_libraries("AvsCore" "Winmm.lib" "Vfw32.lib" "Msacm32.lib" "Gdi32.lib" "User32.lib" "Advapi32.lib" "Ole32.lib")

# Add Boost
set(BOOST_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/boost/include)
#set(BOOST_LIBRARYDIR ${CMAKE_CURRENT_SOURCE_DIR}/boost/lib)
set(Boost_DETAILED_FAILURE_MSG   ON)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME     OFF)
FIND_PACKAGE(Boost 1.55)
add_definitions("/D BOOST_POOL_NO_MT")  # Disable internal synchroniztaion in boost's pool library
include_directories( ${Boost_INCLUDE_DIR} )
#target_link_libraries( "AvsCore" ${Boost_LIBRARIES} )

if (MSVC_IDE)    
  # Copy output to a common folder for easy deployment
  add_custom_command(
    TARGET AvsCore
    POST_BUILD
    COMMAND xcopy /Y \"$(TargetPath)\" \"${CMAKE_BINARY_DIR}/Output\"
  )
endif()

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

# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

# Create library
project("AvsCore")
Include("Files.cmake")
add_library("AvsCore" SHARED ${AvsCore_Sources})
set_target_properties("AvsCore" PROPERTIES "OUTPUT_NAME" "AviSynth")

IF( MSVC_IDE )  # Check for Visual Studio
  # Enable MASM-style assembler support
  enable_language(ASM_MASM)
ENDIF()

# Specify include directories
target_include_directories("AvsCore" PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Specify preprocessor definitions
target_compile_definitions("AvsCore" PRIVATE BUILDING_AVSCORE)

# Windows DLL dependencies 
target_link_libraries("AvsCore" "Winmm.lib" "Vfw32.lib" "Msacm32.lib" "Gdi32.lib" "User32.lib" "Advapi32.lib" "Ole32.lib")

# Add Boost
set(BOOST_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/boost/include)
set(BOOST_LIBRARYDIR ${CMAKE_CURRENT_SOURCE_DIR}/boost/lib)
set(Boost_DETAILED_FAILURE_MSG   ON)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME     OFF)
FIND_PACKAGE( Boost 1.54 COMPONENTS Thread System Date_Time Chrono atomic REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )
target_link_libraries( "AvsCore" ${Boost_LIBRARIES} )

if (MSVC_IDE)    
  # Copy output to a common folder for easy deployment
  add_custom_command(
    TARGET AvsCore
    POST_BUILD
    COMMAND xcopy /Y \"$(TargetPath)\" \"${CMAKE_BINARY_DIR}/Output\"
  )
endif()
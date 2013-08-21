# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

# Find all the input files
Include("Files.cmake")

IF( MSVC_IDE )  # Check for Visual Studio
  # Enable MASM-style assembler support
  enable_language(ASM_MASM)
ENDIF()

# Create library project
add_library("AvsCore" SHARED ${AvsCore_Sources})

# Specify include directories
target_include_directories("AvsCore" PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Specify preprocessor definitions
target_compile_definitions("AvsCore" PRIVATE AVISYNTH_CORE AVISYNTH_C_EXPORTS)

# Windows DLL dependencies 
target_link_libraries("AvsCore" "Winmm.lib" "Vfw32.lib" "Msacm32.lib" "Gdi32.lib" "User32.lib" "Advapi32.lib" "Ole32.lib")
set_target_properties("AvsCore" PROPERTIES "OUTPUT_NAME" "AviSynth")

# Add source dependencies
add_subdirectory("3rd_party/pfc")
add_subdirectory("3rd_party/SoftWire")
add_subdirectory("3rd_party/SoundTouch")
target_link_libraries("AvsCore" "SoundTouch" "PFC" "SoftWire")

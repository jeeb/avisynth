# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

set(PluginName "VDubFilter")
set(ProjectName "Plugin${PluginName}")

# Create library
project(${ProjectName})
list (APPEND SourceFiles
    "VDubFilter.cpp"
    "GetCPUFlags.cpp"
    "ScriptInterpreter.h"
    "ScriptValue.h"
    "VBitmap.h"
    "vdplugin.h"
    "vdvideofilt.h"
)
add_library(${ProjectName} SHARED ${SourceFiles})
set_target_properties(${ProjectName} PROPERTIES "OUTPUT_NAME" ${PluginName})

# Library dependencies 
target_link_libraries(${ProjectName} "User32.lib")

# Include directories
target_include_directories(${ProjectName} PRIVATE ${AvsCore_SOURCE_DIR})

if (MSVC_IDE)    
  # Copy output to a common folder for easy deployment
  add_custom_command(
    TARGET ${ProjectName}
    POST_BUILD
    COMMAND xcopy /Y \"$(TargetPath)\" \"${CMAKE_BINARY_DIR}/Output/plugins\"
  )
endif()

INSTALL(TARGETS "${ProjectName}"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/avisynth")

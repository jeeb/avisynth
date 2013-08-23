# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

set(PluginName "ImageSeq")
set(ProjectName "Plugin${PluginName}")

# Create library
project(${ProjectName})
list (APPEND SourceFiles
    "ImageSeq.cpp"
    "ImageSeq.h"
    "ImageReader.cpp"
    "ImageWriter.cpp"
)
add_library(${ProjectName} SHARED ${SourceFiles})
set_target_properties(${ProjectName} PROPERTIES "OUTPUT_NAME" ${PluginName})

# Specify include directories
target_include_directories(${ProjectName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/DevIL ${AvsCore_SOURCE_DIR})

# Windows DLL dependencies 
target_link_libraries(${ProjectName} "${CMAKE_CURRENT_SOURCE_DIR}/lib/DevIL/DevIL.lib")

# Copy binary dependencies to a common folder for easy deployment
configure_file("${PROJECT_SOURCE_DIR}/lib/DevIL/DevIL.dll" "${CMAKE_BINARY_DIR}/Output/system/DevIL.dll" COPYONLY)
if (MSVC_IDE)    
  # Copy output to a common folder for easy deployment
  add_custom_command(
    TARGET ${ProjectName}
    POST_BUILD
    COMMAND xcopy /Y \"$(TargetPath)\" \"${CMAKE_BINARY_DIR}/Output/plugins\"
  )
endif()
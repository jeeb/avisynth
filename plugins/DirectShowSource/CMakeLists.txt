# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

set(PluginName "DirectShowSource")
set(ProjectName "Plugin${PluginName}")

# We need these variables set by the user to compile successfully
set(DSHOWSRC_BASECLASSES_PATH "C:/Program Files/Microsoft SDKs/Windows/v7.0/Samples/multimedia/directshow/baseclasses" CACHE STRING "Folder path to the DirectShow example baseclasses.")
#set(DSHOWSRC_BASECLASSES_PATH "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Samples/multimedia/directshow/baseclasses" CACHE STRING "Folder path to the DirectShow example baseclasses.")
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
set(DSHOWSRC_BASECLASSES_LIB "c:/Program Files/Microsoft SDKs/Windows/v7.0/Samples/multimedia/directshow/baseclasses/Release_MBCS/strmbase.lib" CACHE FILEPATH "File path to the DirectShow example baseclasses precompiled static library ('strmbase.lib').")
else()
set(DSHOWSRC_BASECLASSES_LIB "c:/Program Files/Microsoft SDKs/Windows/v7.0/Samples/multimedia/directshow/baseclasses/x64/Release_MBCS/strmbase.lib" CACHE FILEPATH "File path to the DirectShow example baseclasses precompiled static library ('strmbase.lib').")
endif()
set(DSHOWSRC_DX_INCLUDE_PATH "C:/Program Files/Microsoft DirectX SDK (August 2009)/Include" CACHE STRING "Include folder path to the DirectX headers.")

# Create library
project(${ProjectName})
list (APPEND SourceFiles
    "directshow_source.cpp"
    "directshow_source.h"
)
add_library(${ProjectName} SHARED ${SourceFiles})
set_target_properties(${ProjectName} PROPERTIES "OUTPUT_NAME" ${PluginName})

# Library dependencies 
target_link_libraries(${ProjectName} "Winmm.lib" "Quartz.lib" "Ole32.lib" "User32.lib" "Oleaut32.lib" "Advapi32.lib" ${DSHOWSRC_BASECLASSES_LIB})

# Allow back permissive C++ Standard conformance, because old DX headers would fail to compile otherwise
if( MSVC_IDE )  # Check for Visual Studio
remove_definitions("/permissive-")
endif()



# Include directories
target_include_directories(${ProjectName} PRIVATE ${AvsCore_SOURCE_DIR} ${DSHOWSRC_BASECLASSES_PATH} ${DSHOWSRC_DX_INCLUDE_PATH})

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

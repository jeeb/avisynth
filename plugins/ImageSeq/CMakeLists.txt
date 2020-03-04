# We need CMake 3.8 at least, because we require
# CMAKE_CXX_STANDARD to be set to C++17.
CMAKE_MINIMUM_REQUIRED( VERSION 3.8.2 )

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
if (MINGW)
  set_target_properties(${ProjectName} PROPERTIES PREFIX "")
  set_target_properties(${ProjectName} PROPERTIES IMPORT_PREFIX "")
endif()

if( CMAKE_SIZEOF_VOID_P EQUAL 4 ) # 32-bit
  # Specify include directories
  target_include_directories(${ProjectName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/DevIL_x86 ${AvsCore_SOURCE_DIR})
  # Windows DLL dependencies 
  target_link_libraries(${ProjectName} "${CMAKE_CURRENT_SOURCE_DIR}/lib/DevIL_x86/DevIL.lib")
  # Copy binary dependencies to a common folder for easy deployment
  configure_file("${PROJECT_SOURCE_DIR}/lib/DevIL_x86/DevIL.dll" "${CMAKE_BINARY_DIR}/Output/system/DevIL.dll" COPYONLY)
else()   # 64-bit
  # Specify include directories
  target_include_directories(${ProjectName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/DevIL_x64 ${AvsCore_SOURCE_DIR})
  # Windows DLL dependencies 
  target_link_libraries(${ProjectName} "${CMAKE_CURRENT_SOURCE_DIR}/lib/DevIL_x64/DevIL.lib")
  # Copy binary dependencies to a common folder for easy deployment
  configure_file("${PROJECT_SOURCE_DIR}/lib/DevIL_x64/DevIL.dll" "${CMAKE_BINARY_DIR}/Output/system/DevIL.dll" COPYONLY)
endif()

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

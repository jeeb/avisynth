# We need CMake 3.8 at least, because we require
# CMAKE_CXX_STANDARD to be set to C++17.
CMAKE_MINIMUM_REQUIRED( VERSION 3.8.2 )

# Find all the input files
FILE(GLOB PFC_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*.cpp"
  "*.h"
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") 
  SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-attributes " )
  SET( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wno-attributes " )
endif()

# This source file must not be compiled
LIST(REMOVE_ITEM PFC_Sources "ptr_list.cpp")

# Create library project
add_library("PFC" STATIC ${PFC_Sources})

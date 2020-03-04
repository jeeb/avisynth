# We need CMake 3.8 at least, because we require
# CMAKE_CXX_STANDARD to be set to C++17.
CMAKE_MINIMUM_REQUIRED( VERSION 3.8.2 )

# Find all the input files
FILE(GLOB PFC_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*.cpp"
  "*.h"
)

# This source file must not be compiled
LIST(REMOVE_ITEM PFC_Sources "ptr_list.cpp")

# Create library project
add_library("PFC" STATIC ${PFC_Sources})

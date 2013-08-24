# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

# Find all the input files
FILE(GLOB PFC_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*.cpp"
  "*.h"
)

# This source file must not be compiled
LIST(REMOVE_ITEM PFC_Sources "ptr_list.cpp")

# Create library project
add_library("PFC" STATIC ${PFC_Sources})

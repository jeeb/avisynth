# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

# Find all the input files
FILE(GLOB SoftWire_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*.cpp"
  "*.hpp"
)

# Create library project
add_library("SoftWire" STATIC ${SoftWire_Sources})

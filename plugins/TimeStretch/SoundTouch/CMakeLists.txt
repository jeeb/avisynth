# We need CMake 2.8.11 at least, because we use CMake features
# "Target Usage Requirements" and "Generator Toolset selection"
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.11 )

# Find all the input files
FILE(GLOB SoundTouch_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*.cpp"
  "*.h"
)

# Create library project
add_library("SoundTouch" STATIC ${SoundTouch_Sources})

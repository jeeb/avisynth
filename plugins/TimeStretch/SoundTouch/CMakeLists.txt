# We need CMake 3.8 at least, because we require
# CMAKE_CXX_STANDARD to be set to C++17.
CMAKE_MINIMUM_REQUIRED( VERSION 3.8.2 )

# Find all the input files
FILE(GLOB SoundTouch_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*.cpp"
  "*.h"
)

# Create library project
add_library("SoundTouch" STATIC ${SoundTouch_Sources})

cmake_minimum_required(VERSION 3.12)
project(sound)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(lib/soundy)

# Create the executable
add_executable(sinewave test/sinewave.cpp)
add_executable(box      test/box.cpp)

# Link the required libraries
target_link_libraries(sinewave PRIVATE soundy asound ${EXTRA_LIBS})
target_link_libraries(box      PRIVATE soundy asound ${EXTRA_LIBS})

# Set compiler flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Debug build")
    # Add debug-specific flags, settings, or definitions
    target_compile_definitions(sinewave PRIVATE DEBUG_MODE)
    target_compile_definitions(box      PRIVATE DEBUG_MODE)
    # Add other debug-specific settings
else()
    message("Release build")
    # Add release-specific flags, settings, or definitions
    # Add other release-specific settings
endif()

cmake_minimum_required(VERSION 3.12)
project(sound)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Define your source files
set(SOUND_LIB_SOURCES 
    lib/alsa/src/device.cpp
    # Add more source files if needed
)

# Create a library from the ALSA source files
add_library(soundy STATIC ${SOUND_LIB_SOURCES})

# Add an include directory for the library
target_include_directories(soundy PUBLIC lib/alsa/include)

# Define your source files
set(SOUND_EXE_SOURCES 
    src/main.cpp
    # Add more source files if needed
)

# Find and link ALSA library
find_library(ALSA_LIB asound)
if (ALSA_LIB)
    message(STATUS "Found ALSA: ${ALSA_LIB}")
    list(APPEND EXTRA_LIBS ${ALSA_LIB})
else ()
    message(FATAL_ERROR "ALSA not found.")
endif ()

# Create the executable
add_executable(sound ${SOUND_EXE_SOURCES})

# Add an include directory for the executable project
target_include_directories(sound PUBLIC include)

# Link the required libraries
target_link_libraries(sound PRIVATE soundy ${EXTRA_LIBS})

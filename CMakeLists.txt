cmake_minimum_required(VERSION 3.12)
project(sound)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(lib/soundy)

# Create the executable
add_executable(sinewave test/sinewave.cpp)
add_executable(synth    test/synth.cpp)

# Link the required libraries
target_link_libraries(sinewave PRIVATE soundy ${EXTRA_LIBS})
target_link_libraries(synth    PRIVATE soundy ${EXTRA_LIBS})

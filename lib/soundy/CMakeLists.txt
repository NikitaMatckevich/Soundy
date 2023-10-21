# Find and link ALSA library
find_library(ALSA_LIB asound)
if (ALSA_LIB)
    message(STATUS "Found ALSA: ${ALSA_LIB}")
    list(APPEND EXTRA_LIBS ${ALSA_LIB})
else ()
    message(FATAL_ERROR "ALSA not found.")
endif ()

# Define soundy library name
set(TARGET_ID soundy)

# Define your source files
set(TARGET_SOURCES 
    src/device.cpp
    # Add more source files if needed
)

# Create a library from the ALSA source files
add_library(${TARGET_ID} SHARED ${TARGET_SOURCES})

# Add an include directory for the library
target_include_directories(${TARGET_ID} PUBLIC include)

# Link the required libraries
target_link_libraries(${TARGET_ID} PRIVATE ${EXTRA_LIBS})

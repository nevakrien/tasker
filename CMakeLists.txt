cmake_minimum_required(VERSION 3.10)

# Project name
project(Tasker LANGUAGES C)

# Define output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(include)

# Define source files
set(SRC_INTRUPT_CLEANUP src/intrupt_cleanup.c)
set(SRC_HELLO_WORLD src/hello_world.c)
set(SRC_WORKER src/worker.c)
set(SRC_TEST test.c)

# Platform-specific flags and libraries
if(WIN32)
    if(MSVC)
        # MSVC-specific flags
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /Zi")
        set(PLATFORM_LIBS ws2_32 Iphlpapi)
    else()
        # Clang or GCC on Windows
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g3")
        set(PLATFORM_LIBS ws2_32 Iphlpapi)
    endif()
    set(EXE_SUFFIX ".exe") # Windows executables need .exe suffix
else()
    # GCC/Clang on Unix-like platforms
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g3")
    set(PLATFORM_LIBS "")
    set(EXE_SUFFIX "") # No suffix on Unix-like platforms
endif()

# Create intrupt_cleanup object library (reusable)
add_library(intrupt_cleanup OBJECT ${SRC_INTRUPT_CLEANUP})

# Hello world binary
add_executable(hello_world${EXE_SUFFIX} ${SRC_HELLO_WORLD})
target_link_libraries(hello_world${EXE_SUFFIX} ${PLATFORM_LIBS})

# Worker binary
add_executable(worker${EXE_SUFFIX} ${SRC_WORKER})
target_link_libraries(worker${EXE_SUFFIX} ${PLATFORM_LIBS})

# Test binary
add_executable(test${EXE_SUFFIX} ${SRC_TEST} $<TARGET_OBJECTS:intrupt_cleanup>)
target_link_libraries(test${EXE_SUFFIX} ${PLATFORM_LIBS})

# Define the "run" custom target
if(WIN32)
    add_custom_target(run
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\\test${EXE_SUFFIX} 6969 4242
        DEPENDS test${EXE_SUFFIX} worker${EXE_SUFFIX}
        COMMENT "Running the test program on Windows..."
    )
else()
    add_custom_target(run
        COMMAND ./bin/test${EXE_SUFFIX} 6969 4242
        DEPENDS test${EXE_SUFFIX} worker${EXE_SUFFIX}
        COMMENT "Running the test program on Unix..."
    )
endif()

cmake_minimum_required(VERSION 3.10)
project(Raytracing VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/appsrc/include)

# Source files
set(MATH_SOURCES
    appsrc/src/Math/vec3.cpp
    appsrc/src/Math/ray.cpp
    appsrc/src/Math/sphere.cpp
    appsrc/src/Math/hittablelist.cpp
    appsrc/src/Math/camera.cpp
)

set(MATH_HEADERS
    appsrc/include/Math/vec3.h
    appsrc/include/Math/ray.h
    appsrc/include/Math/hittable.h
    appsrc/include/Math/sphere.h
    appsrc/include/Math/hittablelist.h
    appsrc/include/Math/camera.h
    appsrc/include/Math/material.h
)

# Create executable
add_executable(raytracing
    main.cpp
    ${MATH_SOURCES}
    ${MATH_HEADERS}
)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(raytracing PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# Install target
install(TARGETS raytracing DESTINATION bin)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
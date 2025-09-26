cmake_minimum_required(VERSION 3.11)
project(Raytracing VERSION 1.0.0 LANGUAGES CXX)

# Handle CMake policy compatibility for dependencies
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# Set minimum policy version to handle GLM compatibility
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)


# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(OpenMP)

# Try to find GLM
find_package(glm QUIET)
if(NOT glm_FOUND)
    # Download GLM if not found
    include(FetchContent)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
    )
        
    FetchContent_MakeAvailable(glm)
endif()

# Download and build Raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
FetchContent_MakeAvailable(raylib)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
    # Add SIMD and modern CPU optimization flags
    if(CMAKE_BUILD_TYPE MATCHES Release)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mavx2 -mfma")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /arch:AVX2")
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

set(INTERACTIVE_SOURCES
    ${MATH_SOURCES}
    appsrc/src/Math/display.cpp
)

set(MATH_HEADERS
    appsrc/include/Math/vec3.h
    appsrc/include/Math/modern_vec3.h
    appsrc/include/Math/modern_sphere.h
    appsrc/include/Math/simd_batch.h
    appsrc/include/Math/performance.h
    appsrc/include/Math/display.h
    appsrc/include/Math/ray.h
    appsrc/include/Math/hittable.h
    appsrc/include/Math/sphere.h
    appsrc/include/Math/hittablelist.h
    appsrc/include/Math/camera.h
    appsrc/include/Math/material.h
)

# Create executables
add_executable(raytracing
    main.cpp
    ${MATH_SOURCES}
    ${MATH_HEADERS}
)

add_executable(raytracing_modern
    main_modern.cpp
    ${MATH_SOURCES}
    ${MATH_HEADERS}
)

add_executable(raytracing_simd
    main_simd.cpp
    ${MATH_SOURCES}
    ${MATH_HEADERS}
)

add_executable(raytracing_simple_perf
    main_simple_perf.cpp
    ${MATH_SOURCES}
    ${MATH_HEADERS}
)

add_executable(raytracing_optimized
    main_optimized.cpp
    ${MATH_SOURCES}
    ${MATH_HEADERS}
)

add_executable(raytracing_interactive_simple
    main_interactive_simple.cpp
    ${MATH_SOURCES}
    ${MATH_HEADERS}
)

add_executable(raytracing_interactive
    main_interactive.cpp
    ${INTERACTIVE_SOURCES}
    ${MATH_HEADERS}
)

# Link libraries
target_link_libraries(raytracing PRIVATE glm::glm)
target_link_libraries(raytracing_modern PRIVATE glm::glm)
target_link_libraries(raytracing_simd PRIVATE glm::glm)
target_link_libraries(raytracing_simple_perf PRIVATE glm::glm)
target_link_libraries(raytracing_optimized PRIVATE glm::glm)
target_link_libraries(raytracing_interactive_simple PRIVATE glm::glm raylib)
target_link_libraries(raytracing_interactive PRIVATE glm::glm raylib)

# Add Raylib include path for interactive targets
target_include_directories(raytracing_interactive_simple PRIVATE ${raylib_SOURCE_DIR}/src)
target_include_directories(raytracing_interactive PRIVATE ${raylib_SOURCE_DIR}/src)

if(OpenMP_CXX_FOUND)
    target_link_libraries(raytracing PRIVATE OpenMP::OpenMP_CXX)
    target_link_libraries(raytracing_modern PRIVATE OpenMP::OpenMP_CXX)
    target_link_libraries(raytracing_simd PRIVATE OpenMP::OpenMP_CXX)
    target_link_libraries(raytracing_optimized PRIVATE OpenMP::OpenMP_CXX)
    target_link_libraries(raytracing_interactive_simple PRIVATE OpenMP::OpenMP_CXX)
    target_link_libraries(raytracing_interactive PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(raytracing PRIVATE USE_OPENMP)
    target_compile_definitions(raytracing_modern PRIVATE USE_OPENMP)
    target_compile_definitions(raytracing_simd PRIVATE USE_OPENMP)
    target_compile_definitions(raytracing_optimized PRIVATE USE_OPENMP)
    target_compile_definitions(raytracing_interactive_simple PRIVATE USE_OPENMP)
    target_compile_definitions(raytracing_interactive PRIVATE USE_OPENMP)
endif()

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
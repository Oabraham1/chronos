cmake_minimum_required(VERSION 3.10)
project(Chronos VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories for all targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(APPLE)
    set(OpenCL_INCLUDE_DIRS "/System/Library/Frameworks/OpenCL.framework/Headers")
    set(OpenCL_LIBRARIES "-framework OpenCL")
else()
    # For non-macOS, use standard FindOpenCL
    if(SKIP_OPENCL_TESTS)
        # Make OpenCL optional when tests are skipped
        find_package(OpenCL QUIET)
        if(NOT OpenCL_FOUND)
            message(STATUS "OpenCL not found, but continuing with SKIP_OPENCL_TESTS enabled")
            set(OpenCL_INCLUDE_DIRS "")
            set(OpenCL_LIBRARIES "")
        endif()
    else()
        find_package(OpenCL REQUIRED)
    endif()
endif()

# Define include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCL_INCLUDE_DIRS}
)

# Source files for the Chronos library
set(CHRONOS_SOURCES
    src/partitioner.cpp
    src/chronos_utils.cpp
    src/core/device_info.cpp
    src/core/gpu_partition.cpp
    src/utils/lock_file.cpp
    src/utils/time_utils.cpp
)

# Add platform-specific sources
if(WIN32)
    list(APPEND CHRONOS_SOURCES src/platform/windows_platform.cpp)
else()
    list(APPEND CHRONOS_SOURCES src/platform/unix_platform.cpp)
endif()



# Create shared library
add_library(chronos SHARED ${CHRONOS_SOURCES})
target_link_libraries(chronos ${OpenCL_LIBRARIES})

# Create static library
add_library(chronos_static STATIC ${CHRONOS_SOURCES})
target_link_libraries(chronos_static ${OpenCL_LIBRARIES})
set_target_properties(chronos_static PROPERTIES OUTPUT_NAME chronos)

# Add platform-specific libraries to the library
if(APPLE)
    # Add frameworks needed on macOS
    find_library(CORE_FOUNDATION CoreFoundation)
    target_link_libraries(chronos ${CORE_FOUNDATION})
    target_link_libraries(chronos_static ${CORE_FOUNDATION})
else()
    # Add pthread on Unix/Linux
    find_package(Threads REQUIRED)
    target_link_libraries(chronos Threads::Threads)
    target_link_libraries(chronos_static Threads::Threads)
endif()

# CLI application
add_executable(chronos_cli apps/cli/main.cpp apps/cli/commands.cpp)
target_link_libraries(chronos_cli chronos)

# Option to build tests
option(BUILD_TESTS "Build test programs" OFF)
option(SKIP_OPENCL_TESTS "Skip tests that require OpenCL devices" OFF)

# When SKIP_OPENCL_TESTS is enabled, define a preprocessor symbol
if(SKIP_OPENCL_TESTS)
    add_definitions(-DSKIP_OPENCL_TESTS)
endif()

if(BUILD_TESTS)
    enable_testing()
    
    # If using Catch2, download it
    if(BUILD_TESTING)
        # Add Catch2 v2 single header approach
        set(CATCH_HEADER_DIR "${CMAKE_BINARY_DIR}/catch2")
        file(MAKE_DIRECTORY ${CATCH_HEADER_DIR}/catch2)

        # Download the single header file
        file(DOWNLOAD
            "https://github.com/catchorg/Catch2/releases/download/v2.13.9/catch.hpp"
            "${CATCH_HEADER_DIR}/catch2/catch.hpp"
            STATUS DOWNLOAD_STATUS
        )
        list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
        if(NOT STATUS_CODE EQUAL 0)
            message(FATAL_ERROR "Failed to download Catch2")
        endif()

        include_directories(${CATCH_HEADER_DIR})
    endif()
    
    # Add tests directory
    add_subdirectory(tests)
endif()

# Option to build examples
option(BUILD_EXAMPLES "Build example programs" OFF)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
install(TARGETS chronos chronos_static chronos_cli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY include/ DESTINATION include/chronos)

# Generate documentation if Doxygen is available
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    # Force Doxygen documentation to be in the root directory
    set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs)
    set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)

    # Make sure the docs directory exists
    file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

    # Create a Doxyfile directly
    set(DOXYFILE_OUT ${CMAKE_BINARY_DIR}/Doxyfile)
    file(WRITE ${DOXYFILE_OUT}
        "PROJECT_NAME = \"Chronos GPU Partitioner\"\n"
        "OUTPUT_DIRECTORY = ${DOXYGEN_OUTPUT_DIR}\n"
        "INPUT = ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src\n"
        "RECURSIVE = YES\n"
        "EXTRACT_ALL = YES\n"
        "EXTRACT_PRIVATE = YES\n"
        "EXTRACT_STATIC = YES\n"
        "GENERATE_LATEX = NO\n"
        "HAVE_DOT = YES\n"
        "UML_LOOK = YES\n"
        "CALL_GRAPH = YES\n"
        "CALLER_GRAPH = YES\n"
    )

    # Add a custom target to generate the documentation
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen in ${DOXYGEN_OUTPUT_DIR}"
        VERBATIM
    )

    # Make it clear how to generate docs
    message(STATUS "Documentation can be generated using 'make docs'")
endif()

# Add a custom target to create a version.h file
file(WRITE ${CMAKE_BINARY_DIR}/generated/version.h
    "#ifndef CHRONOS_VERSION_H\n"
    "#define CHRONOS_VERSION_H\n\n"
    "#define CHRONOS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR}\n"
    "#define CHRONOS_VERSION_MINOR ${PROJECT_VERSION_MINOR}\n"
    "#define CHRONOS_VERSION_PATCH ${PROJECT_VERSION_PATCH}\n"
    "#define CHRONOS_VERSION \"${PROJECT_VERSION}\"\n\n"
    "#endif // CHRONOS_VERSION_H\n"
)
include_directories(${CMAKE_BINARY_DIR}/generated)

# Print configuration summary
message(STATUS "Chronos GPU Partitioner Configuration:")
message(STATUS "  Version:            ${PROJECT_VERSION}")
message(STATUS "  Build type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "  OpenCL found:       ${OpenCL_FOUND}")
message(STATUS "  OpenCL libraries:   ${OpenCL_LIBRARIES}")
message(STATUS "  Build tests:        ${BUILD_TESTS}")
message(STATUS "  Skip OpenCL tests:  ${SKIP_OPENCL_TESTS}")
message(STATUS "  Build examples:     ${BUILD_EXAMPLES}")
message(STATUS "  Install prefix:     ${CMAKE_INSTALL_PREFIX}")
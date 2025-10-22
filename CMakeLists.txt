cmake_minimum_required(VERSION 3.10)
project(Chronos VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(APPLE)
    set(OpenCL_INCLUDE_DIRS "/System/Library/Frameworks/OpenCL.framework/Headers")
    set(OpenCL_LIBRARIES "-framework OpenCL")
    set(OpenCL_FOUND TRUE)
else()
    # Always search quietly first
    find_package(OpenCL QUIET)

    if(NOT OpenCL_FOUND)
        if(SKIP_OPENCL_TESTS)
            message(STATUS "OpenCL not found, but continuing with SKIP_OPENCL_TESTS enabled")
            set(OpenCL_INCLUDE_DIRS "")
            set(OpenCL_LIBRARIES "")
        else()
            message(WARNING "OpenCL not found. Building without OpenCL support.")
            message(WARNING "Install OpenCL development packages or set -DSKIP_OPENCL_TESTS=ON")
            set(OpenCL_INCLUDE_DIRS "")
            set(OpenCL_LIBRARIES "")
            set(SKIP_OPENCL_TESTS ON)
            add_definitions(-DSKIP_OPENCL_TESTS)
        endif()
    endif()
endif()

find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
    message(STATUS "yaml-cpp found - Configuration management will be enabled")
    set(CHRONOS_CONFIG_ENABLED ON)
    add_definitions(-DCHRONOS_CONFIG_ENABLED)
else()
    message(WARNING "yaml-cpp not found - Configuration management will be disabled")
    set(CHRONOS_CONFIG_ENABLED OFF)
endif()

set(CHRONOS_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCL_INCLUDE_DIRS}
)

if(CHRONOS_CONFIG_ENABLED)
    list(APPEND CHRONOS_INCLUDE_DIRS ${YAML_CPP_INCLUDE_DIR})
endif()

include_directories(${CHRONOS_INCLUDE_DIRS})

set(CHRONOS_SOURCES
    src/partitioner.cpp
    src/chronos_utils.cpp
    src/chronos_c.cpp
    src/core/device_info.cpp
    src/core/gpu_partition.cpp
    src/core/memory_enforcer.cpp
    src/utils/lock_file.cpp
    src/utils/time_utils.cpp
)

if(CHRONOS_CONFIG_ENABLED)
    list(APPEND CHRONOS_SOURCES src/config/config_manager.cpp)
endif()

if(WIN32)
    list(APPEND CHRONOS_SOURCES src/platform/windows_platform.cpp)
else()
    list(APPEND CHRONOS_SOURCES src/platform/unix_platform.cpp)
endif()

add_library(chronos SHARED ${CHRONOS_SOURCES})
set(CHRONOS_LINK_LIBRARIES ${OpenCL_LIBRARIES})
if(CHRONOS_CONFIG_ENABLED)
    list(APPEND CHRONOS_LINK_LIBRARIES yaml-cpp)
endif()
target_link_libraries(chronos ${CHRONOS_LINK_LIBRARIES})

add_library(chronos_static STATIC ${CHRONOS_SOURCES})
target_link_libraries(chronos_static ${CHRONOS_LINK_LIBRARIES})
set_target_properties(chronos_static PROPERTIES OUTPUT_NAME chronos)

if(APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    target_link_libraries(chronos ${CORE_FOUNDATION})
    target_link_libraries(chronos_static ${CORE_FOUNDATION})
else()
    find_package(Threads REQUIRED)
    target_link_libraries(chronos Threads::Threads)
    target_link_libraries(chronos_static Threads::Threads)
endif()

add_executable(chronos_cli apps/cli/main.cpp apps/cli/commands.cpp)
target_link_libraries(chronos_cli chronos)

option(BUILD_TESTS "Build test programs" OFF)
option(SKIP_OPENCL_TESTS "Skip tests that require OpenCL devices" OFF)

if(SKIP_OPENCL_TESTS)
    add_definitions(-DSKIP_OPENCL_TESTS)
endif()

if(BUILD_TESTS)
    enable_testing()

    if(BUILD_TESTING)
        set(CATCH_HEADER_DIR "${CMAKE_BINARY_DIR}/catch2")
        file(MAKE_DIRECTORY ${CATCH_HEADER_DIR}/catch2)

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

    add_subdirectory(tests)
endif()

option(BUILD_EXAMPLES "Build example programs" OFF)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(BUILD_BENCHMARKS "Build benchmark programs" OFF)
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

install(TARGETS chronos chronos_static chronos_cli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include/chronos)

install(FILES include/chronos_c.h DESTINATION include/chronos)

if(CHRONOS_CONFIG_ENABLED)
    install(FILES
        configs/chronos.yaml
        DESTINATION /etc/chronos/
        COMPONENT config
        OPTIONAL
    )
endif()

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs)

    # Generate Doxyfile from Doxyfile.in template
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_BINARY_DIR}/Doxyfile
        @ONLY
    )

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen in ${DOXYGEN_OUTPUT_DIR}"
        VERBATIM
    )

    message(STATUS "Documentation can be generated using 'make docs'")
else()
    message(STATUS "Doxygen not found, documentation target not available")
endif()

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

message(STATUS "")
message(STATUS "Chronos GPU Partitioner Configuration:")
message(STATUS "  Version:            ${PROJECT_VERSION}")
message(STATUS "  Build type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "  OpenCL found:       ${OpenCL_FOUND}")
message(STATUS "  OpenCL libraries:   ${OpenCL_LIBRARIES}")
message(STATUS "  Config management:  ${CHRONOS_CONFIG_ENABLED}")
if(CHRONOS_CONFIG_ENABLED)
    message(STATUS "  yaml-cpp:           Found")
else()
    message(STATUS "  yaml-cpp:           Not found (config disabled)")
endif()
message(STATUS "  Build tests:        ${BUILD_TESTS}")
message(STATUS "  Skip OpenCL tests:  ${SKIP_OPENCL_TESTS}")
message(STATUS "  Build examples:     ${BUILD_EXAMPLES}")
message(STATUS "  Build benchmarks:   ${BUILD_BENCHMARKS}")
message(STATUS "  Install prefix:     ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")

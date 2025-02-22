cmake_minimum_required(VERSION 3.23.0)
project(easyqueue
    DESCRIPTION "A simple queue implementation intended to support flexible applicability with an easy-to-use interface."
    HOMEPAGE_URL "https://github.com/null-default/easyqueue"
    LANGUAGES C)

# Semantic Versioning, for the API version.
set(EASYQUEUE_MAJOR_VERSION 1)  # increment for incompatible API changes
set(EASYQUEUE_MINOR_VERSION 1)  # increment with new backwards-compatible functionality
set(EASYQUEUE_PATCH_VERSION 0)  # increment with backwards-compatible bugfixes
set(EASYQUEUE_APIVERSION ${EASYQUEUE_MAJOR_VERSION}.${EASYQUEUE_MINOR_VERSION})

# Shared Object Versioning compatible with libtool's -version-info, for the ABI version.
#   Ref: https://autotools.io/libtool/version.html
set(EASYQUEUE_CURRENT_VERSION 1)   # increment whenever an interface has been added, removed, or changed
set(EASYQUEUE_REVISION_VERSION 1)  # always increment regardless of changes
set(EASYQUEUE_AGE_VERSION 0)       # only increment if ABI changes are backwards-compatible
math(EXPR EASYQUEUE_SOVERSION "${EASYQUEUE_CURRENT_VERSION} - ${EASYQUEUE_AGE_VERSION}")
set(EASYQUEUE_VERSION ${EASYQUEUE_SOVERSION}.${EASYQUEUE_AGE_VERSION}.${EASYQUEUE_REVISION_VERSION})

# Define any boolean options that will tune the build.
option(EASYQUEUE_BUILD_32 "Build 32-bit binaries instead of 64-bit." OFF)
option(EASYQUEUE_BUILD_UNIT_TESTS "Build unit tests included in the repository. Unit tests use the Unity framework, which must be installed on the system." OFF)

# Allow the fixed-size buffer's capacity to be configured.
set(EASYQUEUE_DEFAULT_FIXED_BUFFER_CAPACITY 32)
if(NOT DEFINED EASYQUEUE_FIXED_BUFFER_CAPACITY)
    set(EASYQUEUE_FIXED_BUFFER_CAPACITY ${EASYQUEUE_DEFAULT_FIXED_BUFFER_CAPACITY})
endif()

# Build a shared object.
add_library(${PROJECT_NAME} SHARED)
target_compile_definitions(${PROJECT_NAME}
    PUBLIC EZQ_FIXED_BUFFER_CAPACITY=${EASYQUEUE_FIXED_BUFFER_CAPACITY})
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        C_STANDARD 90
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON)
target_compile_options(${PROJECT_NAME}
    PRIVATE -Wall -Werror -Wextra -Wpedantic)
target_sources(${PROJECT_NAME}
    PRIVATE src/easyqueue.c
    PUBLIC
        FILE_SET easyqueue_headers
            TYPE HEADERS
            BASE_DIRS include
            FILES include/easyqueue.h)
target_link_options(${PROJECT_NAME} PRIVATE -nostdlib)

# Build a static archive.
add_library(${PROJECT_NAME}_static STATIC)
target_compile_definitions(${PROJECT_NAME}_static
    PUBLIC EZQ_FIXED_BUFFER_CAPACITY=${EASYQUEUE_FIXED_BUFFER_CAPACITY})
set_target_properties(${PROJECT_NAME}_static
    PROPERTIES
        C_STANDARD 90
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON)
target_compile_options(${PROJECT_NAME}_static
    PRIVATE -Wall -Werror -Wextra -Wpedantic)
target_sources(${PROJECT_NAME}_static
    PRIVATE src/easyqueue.c
    PUBLIC
        FILE_SET easyqueue_headers
            TYPE HEADERS
            BASE_DIRS include
            FILES include/easyqueue.h)

# Set additional flags for 32-bit builds.
if(EASYQUEUE_BUILD_32)
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            COMPILE_FLAGS "-m32"
            LINK_FLAGS "-m32")
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -fno-stack-protector) # is this the best option..?

    set_target_properties(${PROJECT_NAME}_static
        PROPERTIES
            COMPILE_FLAGS "-m32"
            LINK_FLAGS "-m32")
    target_compile_options(${PROJECT_NAME}_static
        PRIVATE
            -fno-stack-protector)
endif()

# Example executables that demonstrate usage of the library can be compiled if
# EASYQUEUE_BUILD_EXAMPLES is provided as a CMake variable.
if(EASYQUEUE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# If unit test building is specified, include CTest.
#   NOTE: 32-bit builds of unit tests are not currently supported.
if(EASYQUEUE_BUILD_UNIT_TESTS)
    find_library(UNITY_TESTS
        NAMES unity)
    include(CTest)
    add_executable(easyqueue_unit_tests src/easyqueue.tests.c)
    target_link_libraries(easyqueue_unit_tests
        PRIVATE
            ${PROJECT_NAME}_static
            ${UNITY_TESTS})
    add_test(NAME easyqueue_unit_tests
        COMMAND easyqueue_unit_tests)
endif()

# Set installation rules
install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_static
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    FILE_SET easyqueue_headers)

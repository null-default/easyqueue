cmake_minimum_required(VERSION 3.23.0)
project(easyqueue
        DESCRIPTION "A simple queue implementation intended to support flexible applicability with an easy-to-use interface."
        HOMEPAGE_URL "https://github.com/null-default/easyqueue"
        LANGUAGES C)

# Semantic Versioning, for the API version.
set(EASYQUEUE_MAJOR_VERSION 1)  # increment for incompatible API changes
set(EASYQUEUE_MINOR_VERSION 0)  # increment with new backwards-compatible functionality
set(EASYQUEUE_PATCH_VERSION 0)  # increment with backwards-compatible bugfixes
set(EASYQUEUE_APIVERSION ${EASYQUEUE_MAJOR_VERSION}.${EASYQUEUE_MINOR_VERSION})

# Shared Object Versioning compatible with libtool's -version-info, for the ABI version.
#   Ref: https://autotools.io/libtool/version.html
set(EASYQUEUE_CURRENT_VERSION 1)   # increment whenever an interface has been added, removed, or changed
set(EASYQUEUE_REVISION_VERSION 0)  # always increment regardless of changes
set(EASYQUEUE_AGE_VERSION 0)       # only increment if ABI changes are backwards-compatible
math(EXPR EASYQUEUE_SOVERSION "${EASYQUEUE_CURRENT_VERSION} - ${EASYQUEUE_AGE_VERSION}")
set(EASYQUEUE_VERSION ${EASYQUEUE_SOVERSION}.${EASYQUEUE_AGE_VERSION}.${EASYQUEUE_REVISION_VERSION})

# Allow the fixed-size buffer's capacity to be configured.
set(EASYQUEUE_DEFAULT_FIXED_BUFFER_CAPACITY 32)
if(NOT DEFINED EASYQUEUE_FIXED_BUFFER_CAPACITY)
    set(EASYQUEUE_FIXED_BUFFER_CAPACITY ${EASYQUEUE_DEFAULT_FIXED_BUFFER_CAPACITY})
endif()

# Shared object
add_library(${PROJECT_NAME} SHARED)
target_compile_definitions(${PROJECT_NAME}
        PUBLIC EZQ_FIXED_BUFFER_CAPACITY=${EASYQUEUE_FIXED_BUFFER_CAPACITY})
set_target_properties(${PROJECT_NAME}
        PROPERTIES C_STANDARD 90
                   C_STANDARD_REQUIRED On
                   C_EXTENSIONS Off
                   POSITION_INDEPENDENT_CODE On)
target_compile_options(${PROJECT_NAME}
        PRIVATE -Wall -Werror -Wextra -Wpedantic)
target_sources(${PROJECT_NAME}
        PRIVATE src/easyqueue.c
        PUBLIC FILE_SET easyqueue_headers
            TYPE HEADERS
            BASE_DIRS include
            FILES include/easyqueue.h)
target_link_options(${PROJECT_NAME} PRIVATE -nostdlib)

# Static archive
add_library(${PROJECT_NAME}_static STATIC)
target_compile_definitions(${PROJECT_NAME}_static
        PUBLIC EZQ_FIXED_BUFFER_CAPACITY=${EASYQUEUE_FIXED_BUFFER_CAPACITY})
set_target_properties(${PROJECT_NAME}_static
        PROPERTIES C_STANDARD 90
                   C_STANDARD_REQUIRED On
                   C_EXTENSIONS Off
                   POSITION_INDEPENDENT_CODE On)
target_compile_options(${PROJECT_NAME}_static
        PRIVATE -Wall -Werror -Wextra -Wpedantic)
target_sources(${PROJECT_NAME}_static
        PRIVATE src/easyqueue.c
        PUBLIC FILE_SET easyqueue_headers
            TYPE HEADERS
            BASE_DIRS include
            FILES include/easyqueue.h)

# Example executables that demonstrate usage of the library can be compiled if
# EASYQUEUE_BUILD_EXAMPLES is provided as a CMake variable.
if(EASYQUEUE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Set installation rules
install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_static
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET easyqueue_headers)

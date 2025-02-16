cmake_minimum_required(VERSION 3.20.2)
project(easyqueue
        DESCRIPTION "A simple queue implementation intended to support both performance and flexibility."
        LANGUAGES C)

# Semantic Versioning, for the API version.
set(EASYQUEUE_MAJOR_VERSION 1)  # increment for incompatible API changes
set(EASYQUEUE_MINOR_VERSION 0)  # increment with new backwards-compatible functionality
set(EASYQUEUE_PATCH_VERSION 0)  # increment with backwards-compatible bugfixes
set(EASYQUEUE_APIVERSION ${EASYQUEUE_MAJOR_VERSION}.${EASYQUEUE_MINOR_VERSION})

# Shared Object Versioning compatible with libtool's -version-info, for the
# ABI version.
#   Ref: https://autotools.io/libtool/version.html
set(EASYQUEUE_CURRENT_VERSION 1)   # increment whenever an interface has been added, removed, or changed
set(EASYQUEUE_REVISION_VERSION 0)  # always increment regardless of changes
set(EASYQUEUE_AGE_VERSION 0)       # only increment if ABI changes are backwards-compatible
math(EXPR EASYQUEUE_SOVERSION "${EASYQUEUE_CURRENT_VERSION} - ${EASYQUEUE_AGE_VERSION}")
set(EASYQUEUE_VERSION ${EASYQUEUE_SOVERSION}.${EASYQUEUE_AGE_VERSION}.${EASYQUEUE_REVISION_VERSION})

# Use an old standard to try and maximize portability.
set(CMAKE_C_STANDARD 90)

# Set the generator used for this build.
if(EASYQUEUE_USE_GENERATOR_MAKE)
    set(CMAKE_GENERATOR "Unix Makefiles")
else()
    set(CMAKE_GENERATOR "Ninja")
endif()

add_library(${PROJECT_NAME} STATIC)
target_include_directories(${PROJECT_NAME} PUBLIC include)

add_subdirectory(src)

if(EASYQUEUE_BUILD_EXAMPLES)
    message("Hello world")
    add_subdirectory(examples)
endif()

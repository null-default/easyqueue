include_directories(PUBLIC ${PROJECT_NAME})

# Build the example executable. It is linked with the shared object file generated during the build.
add_executable(easyqueue_example example.c)
target_link_libraries(easyqueue_example PUBLIC ${PROJECT_NAME})
set_target_properties(easyqueue_example PROPERTIES C_STANDARD 90
                                                   C_STANDARD_REQUIRED ON
                                                   C_EXTENSIONS OFF)
target_compile_options(easyqueue_example PRIVATE -Wall -Werror -Wextra -Wpedantic)

# Set additional flags for 32-bit builds.
if(EASYQUEUE_BUILD_32)
    set_target_properties(easyqueue_example
            PROPERTIES COMPILE_FLAGS "-m32"
                       LINK_FLAGS "-m32")
endif()

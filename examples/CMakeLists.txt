include_directories(PUBLIC ${PROJECT_NAME})

add_executable(easyqueue_example example.c)
target_link_libraries(easyqueue_example PUBLIC ${PROJECT_NAME})
set_target_properties(easyqueue_example PROPERTIES C_STANDARD 90
                                                   C_STANDARD_REQUIRED On
                                                   C_EXTENSIONS Off)
target_compile_options(easyqueue_example PRIVATE -Wall -Werror -Wextra -Wpedantic)

if(EASYQUEUE_BUILD_32)
    target_compile_options(easyqueue_example PRIVATE -m32)
endif()

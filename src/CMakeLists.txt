# The library uses as old a C standard as possible to maximize portability.
set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 90
                                                 C_STANDARD_REQUIRED On
                                                 C_EXTENSIONS Off)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra)
target_sources(${PROJECT_NAME} PRIVATE easyqueue.c)

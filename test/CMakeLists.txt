cmake_minimum_required(VERSION 3.17)

project(test_log 
    LANGUAGES C
    VERSION 1.0.0
    DESCRIPTION "log test Project"
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXTENSIONS OFF)

file(GLOB_RECURSE SRC_SOURCES 
        "./test_log.c"
        "../src/*.c"
)

add_executable(test_log ${SRC_SOURCES})

target_include_directories(test_log 
    PRIVATE 
        /usr/local/include 
        ../src
)

target_link_directories(test_log 
    PRIVATE 
        /usr/local/lib
)

target_link_libraries(test_log
    PRIVATE
        pthread
)

# 编译选项
target_compile_options(test_log PRIVATE -Wall -Wextra -Werror -O2)
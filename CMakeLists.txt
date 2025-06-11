cmake_minimum_required(VERSION 3.22)
project(untitled_compiler_project C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wconversion -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,undefined")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

add_executable(untitled_compiler_project
        src/main.c
        src/lexer.c
        src/preprocessor.c
        src/parser.c
        src/memory.c
        src/debug.c
        src/helper_functions.c
        src/common.c
        src/strings.c
        src/hash_table.c
)

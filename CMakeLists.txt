cmake_minimum_required(VERSION 3.22)
project(Compiler C)

set(CMAKE_C_STANDARD 17)

add_executable(Compiler
        src/lexer/lexer.c
        src/lexer/lexer.h
)

add_executable(tests tests/hello_world.c)

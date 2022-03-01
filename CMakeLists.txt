cmake_minimum_required(VERSION 3.12)
project(game)

set(HEADER_FILES
    arena.h
    assert.h
    dynarray.h)

set(SOURCE_FILES
    arena.c
    dynarray.c)

add_executable(game ${HEADER_FILES} ${SOURCE_FILES} main.c)

set_property(TARGET game PROPERTY C_STANDARD 99)

if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(game PRIVATE /W4 /permissive- /DDEBUG)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(game PRIVATE -Wall -Wextra -pedantic -DDEBUG)
elseif (CMAKE_C_COMPILER_ID MATCHES "CLANG")
    target_compile_options(game PRIVATE -Wall -Wextra -pedantic -DDEBUG)
endif()

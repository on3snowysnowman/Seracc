
cmake_minimum_required(VERSION 3.10)
project(SERAC_COMPILER VERSION 1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


file(GLOB_RECURSE SOURCES "src/*.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(Seracc ${SOURCES})

add_custom_command(TARGET Seracc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename
    $<TARGET_FILE:Seracc>
    $<TARGET_FILE_DIR:Seracc>/Seracc.bin
)

target_compile_options(Seracc PRIVATE -Wall -Wno-unused-function -Wextra -std=c++17 -g)

target_include_directories(Seracc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:/usr/local/include>
        $<BUILD_INTERFACE:/usr/include>
)
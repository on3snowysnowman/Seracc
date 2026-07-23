
cmake_minimum_required(VERSION 3.10)
project(SERAC_COMPILER VERSION 1.0 LANGUAGES C)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

add_executable(Seracc ${SOURCES})

# -- Prebuilt static library -- 
add_library(Tundra STATIC IMPORTED)

set_target_properties(Tundra PROPERTIES
    IMPORTED_LOCATION "/usr/local/lib/libTundra.a"
    INTERFACE_INCLUDE_DIRECTORIES "/usr/local/include"
)

target_link_libraries(Seracc PRIVATE Tundra)

# ------------------------------

target_compile_features(Seracc PRIVATE c_std_99)

target_compile_options(Seracc PRIVATE 
    -Wall 
    -Wno-unused-function 
    -Wno-unused-parameter
    -Wextra 
    -g)

target_link_options(Seracc PRIVATE
     -fsanitize=address,undefined
)

target_include_directories(Seracc PRIVATE
    "${CMAKE_SOURCE_DIR}/include")

set_target_properties(Seracc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    OUTPUT_NAME "Seracc.bin"
)

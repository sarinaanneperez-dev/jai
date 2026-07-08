cmake_minimum_required(VERSION 3.16)
project(Jaishi
    VERSION 1.0.0
    DESCRIPTION "Jaishi UCI chess engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
    add_compile_options($<$<CONFIG:Release>:/O2>)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options($<$<CONFIG:Release>:-O3>)
    add_compile_options($<$<CONFIG:Release>:-DNDEBUG>)
endif()

set(JAISHI_CORE_SOURCES
    src/board.cpp
    src/move.cpp
    src/movegen.cpp
    src/search.cpp
    src/evaluation.cpp
    src/zobrist.cpp
    src/tt.cpp
    src/time_manager.cpp
    src/opening_personality.cpp
    src/history.cpp
    src/perft.cpp
    src/uci.cpp
)

add_library(jaishi_core STATIC ${JAISHI_CORE_SOURCES})
target_include_directories(jaishi_core PUBLIC src)

add_executable(jaishi src/main.cpp)
target_link_libraries(jaishi PRIVATE jaishi_core)

enable_testing()

add_executable(jaishi_tests
    tests/perft_tests.cpp
    tests/search_tests.cpp
    tests/evaluation_tests.cpp
)
target_link_libraries(jaishi_tests PRIVATE jaishi_core)
add_test(NAME JaishiTests COMMAND jaishi_tests)

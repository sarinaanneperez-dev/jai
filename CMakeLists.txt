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

# --- Individual test executables ---
add_executable(jaishi_perft_tests tests/perft_tests.cpp)
target_link_libraries(jaishi_perft_tests PRIVATE jaishi_core)
add_test(NAME PerftTests COMMAND jaishi_perft_tests)

add_executable(jaishi_search_tests tests/search_tests.cpp)
target_link_libraries(jaishi_search_tests PRIVATE jaishi_core)
add_test(NAME SearchTests COMMAND jaishi_search_tests)

add_executable(jaishi_evaluation_tests tests/evaluation_tests.cpp)
target_link_libraries(jaishi_evaluation_tests PRIVATE jaishi_core)
add_test(NAME EvaluationTests COMMAND jaishi_evaluation_tests)

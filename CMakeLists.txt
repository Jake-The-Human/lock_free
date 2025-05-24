cmake_minimum_required(VERSION 3.31)
project(lock-free)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.8.1
)

FetchContent_MakeAvailable(Catch2)

add_library(${PROJECT_NAME}
  src/queue.hpp
  src/list.cpp
)

add_executable(tests
  tests/queue_test.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(tests PROPERTIES CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(tests)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain ${PROJECT_NAME})


# BENCHMARKS
# perf stat -e instructions,cycles,cache-misses,cache-references <binary>

add_executable(P1C1 benchmark/P1C1.cpp)
target_link_libraries(P1C1 ${PROJECT_NAME})
set_target_properties(P1C1 PROPERTIES CXX_STANDARD 20)




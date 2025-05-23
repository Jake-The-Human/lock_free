cmake_minimum_required(VERSION 3.31)
project(lock-free)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(${PROJECT_NAME} src/queue.cpp src/list.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

add_executable(tests tests/queue_test.cpp)
set_target_properties(tests PROPERTIES CXX_STANDARD 20)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)

FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(tests)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain ${PROJECT_NAME})


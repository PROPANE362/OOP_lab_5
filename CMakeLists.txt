cmake_minimum_required(VERSION 3.14)
project(pmr_container CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(pmr_lib INTERFACE)
target_include_directories(pmr_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(demo src/main.cpp)
target_link_libraries(demo pmr_lib)

enable_testing()
add_executable(tests tests/test.cpp)
target_link_libraries(tests pmr_lib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(tests)

cmake_minimum_required(VERSION 3.14)

project(infix-parserTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(infix-parser REQUIRED)
  enable_testing()
endif()

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(infix-parser_test source/infix-parser_test.cpp)
target_link_libraries(
    infix-parser_test PRIVATE
    infix-parser::infix-parser
    Catch2::Catch2WithMain
)
target_compile_features(infix-parser_test PRIVATE cxx_std_20)

catch_discover_tests(infix-parser_test)

# ---- End-of-file commands ----

add_folders(Test)

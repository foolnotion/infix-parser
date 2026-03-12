cmake_minimum_required(VERSION 3.14)

project(infix-parserBenchmarks CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(infix-parser REQUIRED)
endif()

find_package(nanobench REQUIRED)

add_executable(infix-parser_bench source/infix-parser_bench.cpp)
target_link_libraries(
    infix-parser_bench PRIVATE
    infix-parser::infix-parser
    nanobench::nanobench
)
target_compile_features(infix-parser_bench PRIVATE cxx_std_20)

add_folders(Benchmark)

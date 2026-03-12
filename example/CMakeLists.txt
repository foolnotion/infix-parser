cmake_minimum_required(VERSION 3.14)

project(infix-parserExamples CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(infix-parser REQUIRED)
endif()

find_package(fmt REQUIRED)

add_custom_target(run-examples)

add_executable(calculator calculator.cpp)
target_link_libraries(calculator PRIVATE infix-parser::infix-parser fmt::fmt)
target_compile_features(calculator PRIVATE cxx_std_20)
add_custom_target(run_calculator COMMAND calculator VERBATIM)
add_dependencies(run_calculator calculator)
add_dependencies(run-examples run_calculator)

add_folders(Example)

cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    infix-parser
    VERSION 0.1.0
    DESCRIPTION "A fast infix expression parser producing reverse-prefix ASTs"
    HOMEPAGE_URL "https://github.com/foolnotion/infix-parser"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    infix-parser_infix-parser
    source/parser.cpp
)
add_library(infix-parser::infix-parser ALIAS infix-parser_infix-parser)

include(GenerateExportHeader)
generate_export_header(
    infix-parser_infix-parser
    BASE_NAME infix-parser
    EXPORT_FILE_NAME export/infix-parser/infix-parser_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(infix-parser_infix-parser PUBLIC INFIX_PARSER_STATIC_DEFINE)
endif()

set_target_properties(
    infix-parser_infix-parser PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME infix-parser
    OUTPUT_NAME infix-parser
)

target_include_directories(
    infix-parser_infix-parser ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    infix-parser_infix-parser SYSTEM
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(infix-parser_infix-parser PUBLIC cxx_std_20)

find_package(FastFloat REQUIRED)
find_package(fmt REQUIRED)
find_package(lexy REQUIRED)
target_link_libraries(infix-parser_infix-parser PRIVATE FastFloat::fast_float fmt::fmt foonathan::lexy)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${infix-parser_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT infix-parser_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of infix-parser"
  )
endif()

include(cmake/dev-mode.cmake)

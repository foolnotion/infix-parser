set(infix-parser_FOUND YES)

# fmt, lexy, and FastFloat are private implementation details —
# consumers only need the standard library headers exposed by ast.hpp / parser.hpp.

if(infix-parser_FOUND)
  include("${CMAKE_CURRENT_LIST_DIR}/infix-parserTargets.cmake")
endif()

// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Bogdan Burlacu <bogdan.burlacu@pm.me>

#ifndef INFIX_PARSER_PARSER_HPP
#define INFIX_PARSER_PARSER_HPP

#include <string>
#include <string_view>
#include <variant>

#include "ast.hpp"
#include "infix-parser/infix-parser_export.hpp"

namespace infix_parser
{

struct parse_error
{
  std::string message;
  std::size_t position = 0;
};

// Parse an infix expression string into a postfix (reverse-prefix) node list.
// Children appear before their parent; for binary ops the right child precedes
// the left.
INFIX_PARSER_EXPORT auto parse(std::string_view infix)
    -> std::variant<expression, parse_error>;

}  // namespace infix_parser

#endif

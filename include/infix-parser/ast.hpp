// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Bogdan Burlacu <bogdan.burlacu@pm.me>

#ifndef INFIX_PARSER_AST_HPP
#define INFIX_PARSER_AST_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace infix_parser
{

enum class node_type : uint8_t
{
  // Terminals
  constant = 0,
  variable,

  // Binary arithmetic (infix)
  add,
  sub,
  mul,
  div,
  pow,

  // Comparison (infix + function-call)
  lt,
  gt,

  // Unary functions
  abs,
  square,
  exp,
  log,
  sin,
  cos,
  tan,
  asin,
  acos,
  atan,
  sinh,
  cosh,
  tanh,
  sqrt,
  cbrt,
  erf,
  sigmoid,  // 1 / (1 + exp(-x))
  softplus,  // ln(1 + exp(x))
  gauss,  // exp(-x^2)

  // Protected unary
  log1p,  // ln(1 + |x|)
  logabs,  // ln(|x|)
  sqrtabs,  // sqrt(|x|)

  // Binary functions (function-call only)
  aq,
  fmin,
  fmax,

  // Protected binary
  powabs,  // |x|^y

  // Ternary
  ternary,  // c*a + (1-c)*b  (smooth)

  // Sentinel
  count
};

struct node
{
  node_type type = node_type::constant;
  uint8_t arity = 0;
  double value = 0.0;
  std::string name;

  node() = default;

  static auto constant(double v) -> node
  {
    node n;
    n.type = node_type::constant;
    n.arity = 0;
    n.value = v;
    return n;
  }

  static auto variable(std::string s) -> node
  {
    node n;
    n.type = node_type::variable;
    n.arity = 0;
    n.name = std::move(s);
    return n;
  }

  static auto function(node_type t, uint8_t a) -> node
  {
    node n;
    n.type = t;
    n.arity = a;
    return n;
  }
};

using expression = std::vector<node>;

}  // namespace infix_parser

#endif  // INFIX_PARSER_AST_HPP

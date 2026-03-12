// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Bogdan Burlacu <bogdan.burlacu@pm.me>

#include <string_view>
#include <variant>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "infix-parser/ast.hpp"
#include "infix-parser/parser.hpp"

using infix_parser::expression;
using infix_parser::node_type;
using infix_parser::parse;
using infix_parser::parse_error;

// Parse and require success, returning the expression.
auto parse_ok(std::string_view s) -> expression
{
  auto result = parse(s);
  REQUIRE(std::holds_alternative<expression>(result));
  return std::get<expression>(result);
}

// ---- Error handling --------------------------------------------------------

TEST_CASE("parse fails on invalid input", "[parser][error]")
{
  CHECK(std::holds_alternative<parse_error>(parse("")));
  CHECK(std::holds_alternative<parse_error>(parse("(")));
  CHECK(std::holds_alternative<parse_error>(parse("1 +")));
  CHECK(std::holds_alternative<parse_error>(parse("* a")));
  // Unknown identifier followed by call syntax leaves trailing tokens
  CHECK(std::holds_alternative<parse_error>(parse("unknown(x)")));
}

// ---- Constants -------------------------------------------------------------

TEST_CASE("integer constant", "[parser][constant]")
{
  auto expr = parse_ok("42");
  REQUIRE(expr.size() == 1);
  CHECK(expr[0].type == node_type::constant);
  CHECK(expr[0].arity == 0);
  CHECK(expr[0].value == Catch::Approx(42.0));
}

TEST_CASE("floating-point constant", "[parser][constant]")
{
  auto expr = parse_ok("3.14");
  REQUIRE(expr.size() == 1);
  CHECK(expr[0].value == Catch::Approx(3.14));
}

TEST_CASE("scientific notation", "[parser][constant]")
{
  SECTION("lowercase e, negative exponent")
  {
    auto expr = parse_ok("6.674e-11");
    REQUIRE(expr.size() == 1);
    CHECK(expr[0].value == Catch::Approx(6.674e-11));
  }
  SECTION("uppercase E, positive exponent")
  {
    auto expr = parse_ok("1.5E+2");
    REQUIRE(expr.size() == 1);
    CHECK(expr[0].value == Catch::Approx(150.0));
  }
  SECTION("no fractional part")
  {
    auto expr = parse_ok("2e3");
    REQUIRE(expr.size() == 1);
    CHECK(expr[0].value == Catch::Approx(2000.0));
  }
}

// ---- Variables -------------------------------------------------------------

TEST_CASE("variable", "[parser][variable]")
{
  auto expr = parse_ok("foo_1");
  REQUIRE(expr.size() == 1);
  CHECK(expr[0].type == node_type::variable);
  CHECK(expr[0].arity == 0);
  CHECK(expr[0].name == "foo_1");
}

// ---- Binary infix operators ------------------------------------------------

// Encoding for a binary leaf expression "a OP b" is [b, a, op(arity=2)].
TEST_CASE("simple binary operators", "[parser][binary]")
{
  auto check = [](std::string_view s, node_type expected_op)
  {
    auto expr = parse_ok(s);
    REQUIRE(expr.size() == 3);
    CHECK(expr[0].type == node_type::variable);  // rhs leaf
    CHECK(expr[1].type == node_type::variable);  // lhs leaf
    CHECK(expr[2].type == expected_op);
    CHECK(expr[2].arity == 2);
  };
  check("a + b", node_type::add);
  check("a - b", node_type::sub);
  check("a * b", node_type::mul);
  check("a / b", node_type::div);
  check("a ^ b", node_type::pow);
  check("a < b", node_type::lt);
  check("a > b", node_type::gt);
}

// ---- Operator precedence ---------------------------------------------------

// a + b*c  →  [c, b, mul, a, add]
TEST_CASE("mul binds tighter than add", "[parser][precedence]")
{
  auto expr = parse_ok("a + b * c");
  REQUIRE(expr.size() == 5);
  CHECK(expr[0].name == "c");
  CHECK(expr[1].name == "b");
  CHECK(expr[2].type == node_type::mul);
  CHECK(expr[3].name == "a");
  CHECK(expr[4].type == node_type::add);
}

// a*b + c  →  [c, b, a, mul, add]
TEST_CASE("mul before add on the left", "[parser][precedence]")
{
  auto expr = parse_ok("a * b + c");
  REQUIRE(expr.size() == 5);
  CHECK(expr[0].name == "c");
  CHECK(expr[1].name == "b");
  CHECK(expr[2].name == "a");
  CHECK(expr[3].type == node_type::mul);
  CHECK(expr[4].type == node_type::add);
}

// (a+b)*c  →  [c, b, a, add, mul]
TEST_CASE("parentheses override precedence", "[parser][precedence]")
{
  auto expr = parse_ok("(a + b) * c");
  REQUIRE(expr.size() == 5);
  CHECK(expr[0].name == "c");
  CHECK(expr[1].name == "b");
  CHECK(expr[2].name == "a");
  CHECK(expr[3].type == node_type::add);
  CHECK(expr[4].type == node_type::mul);
}

// ---- Associativity ---------------------------------------------------------

// a^b^c = a^(b^c)  →  [c, b, pow, a, pow]
TEST_CASE("power is right-associative", "[parser][associativity]")
{
  auto expr = parse_ok("a^b^c");
  REQUIRE(expr.size() == 5);
  CHECK(expr[0].name == "c");
  CHECK(expr[1].name == "b");
  CHECK(expr[2].type == node_type::pow);
  CHECK(expr[3].name == "a");
  CHECK(expr[4].type == node_type::pow);
}

// a-b-c = (a-b)-c  →  [c, b, a, sub, sub]
TEST_CASE("subtraction is left-associative", "[parser][associativity]")
{
  auto expr = parse_ok("a - b - c");
  REQUIRE(expr.size() == 5);
  CHECK(expr[0].name == "c");
  CHECK(expr[1].name == "b");
  CHECK(expr[2].name == "a");
  CHECK(expr[3].type == node_type::sub);
  CHECK(expr[4].type == node_type::sub);
}

// ---- Unary negation --------------------------------------------------------

// -x  →  [x, sub(arity=1)]
TEST_CASE("unary negation", "[parser][unary]")
{
  auto expr = parse_ok("-x");
  REQUIRE(expr.size() == 2);
  CHECK(expr[0].type == node_type::variable);
  CHECK(expr[1].type == node_type::sub);
  CHECK(expr[1].arity == 1);
}

// -(a+b)  →  [b, a, add, sub(arity=1)]
TEST_CASE("unary negation of subexpression", "[parser][unary]")
{
  auto expr = parse_ok("-(a + b)");
  REQUIRE(expr.size() == 4);
  CHECK(expr[2].type == node_type::add);
  CHECK(expr[3].type == node_type::sub);
  CHECK(expr[3].arity == 1);
}

// ---- Unary function calls --------------------------------------------------

TEST_CASE("unary functions", "[parser][unary]")
{
  auto check = [](std::string_view s, node_type expected)
  {
    auto expr = parse_ok(s);
    REQUIRE(expr.size() == 2);
    CHECK(expr[0].type == node_type::variable);
    CHECK(expr[1].type == expected);
    CHECK(expr[1].arity == 1);
  };
  check("abs(x)", node_type::abs);
  check("square(x)", node_type::square);
  check("exp(x)", node_type::exp);
  check("log(x)", node_type::log);
  check("log1p(x)", node_type::log1p);
  check("logabs(x)", node_type::logabs);
  check("sqrt(x)", node_type::sqrt);
  check("sqrtabs(x)", node_type::sqrtabs);
  check("cbrt(x)", node_type::cbrt);
  check("sin(x)", node_type::sin);
  check("cos(x)", node_type::cos);
  check("tan(x)", node_type::tan);
  check("asin(x)", node_type::asin);
  check("acos(x)", node_type::acos);
  check("atan(x)", node_type::atan);
  check("sinh(x)", node_type::sinh);
  check("cosh(x)", node_type::cosh);
  check("tanh(x)", node_type::tanh);
  check("erf(x)", node_type::erf);
  check("sigmoid(x)", node_type::sigmoid);
  check("softplus(x)", node_type::softplus);
  check("gauss(x)", node_type::gauss);
}

// ---- Binary function calls -------------------------------------------------

// fn(x, y)  →  [y, x, op(arity=2)]
TEST_CASE("binary functions", "[parser][binary]")
{
  auto check = [](std::string_view s, node_type expected)
  {
    auto expr = parse_ok(s);
    REQUIRE(expr.size() == 3);
    CHECK(expr[0].name == "y");
    CHECK(expr[1].name == "x");
    CHECK(expr[2].type == expected);
    CHECK(expr[2].arity == 2);
  };
  check("pow(x, y)", node_type::pow);
  check("powabs(x, y)", node_type::powabs);
  check("aq(x, y)", node_type::aq);
  check("min(x, y)", node_type::fmin);
  check("max(x, y)", node_type::fmax);
  check("lt(x, y)", node_type::lt);
  check("gt(x, y)", node_type::gt);
}

// ---- Ternary function call -------------------------------------------------

// if(cond, then, else) → merge_ternary → [else, then, cond, ternary(arity=3)]
TEST_CASE("ternary function", "[parser][ternary]")
{
  auto expr = parse_ok("if(x, y, z)");
  REQUIRE(expr.size() == 4);
  CHECK(expr[0].name == "z");  // else branch
  CHECK(expr[1].name == "y");  // then branch
  CHECK(expr[2].name == "x");  // condition
  CHECK(expr[3].type == node_type::ternary);
  CHECK(expr[3].arity == 3);
}

// ---- Whitespace ------------------------------------------------------------

TEST_CASE("whitespace is insignificant", "[parser]")
{
  auto compact = parse_ok("a+b*c");
  auto spaced = parse_ok("  a  +  b  *  c  ");
  REQUIRE(compact.size() == spaced.size());
  for (std::size_t i = 0; i < compact.size(); ++i) {
    CHECK(compact[i].type == spaced[i].type);
    CHECK(compact[i].name == spaced[i].name);
  }
}

// ---- Composite expressions -------------------------------------------------

// sin(x)^2 + cos(x)^2  →  should produce a 9-node expression
TEST_CASE("sin(x)^2 + cos(x)^2", "[parser][composite]")
{
  auto expr = parse_ok("sin(x)^2 + cos(x)^2");
  // Structure (rhs first):
  //   cos(x)^2:  [2, x, cos, pow]
  //   sin(x)^2:  [2, x, sin, pow]
  //   sum:       [cos_block, sin_block, add]  →  9 nodes total
  REQUIRE(expr.size() == 9);
  CHECK(expr[8].type == node_type::add);
  CHECK(expr[8].arity == 2);
}

// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Bogdan Burlacu <bogdan.burlacu@pm.me>

#include <type_traits>
#include <utility>
#include <vector>

#include "infix-parser/parser.hpp"

#include <fast_float/fast_float.h>
#include <lexy/action/parse.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp>

namespace infix_parser
{
namespace
{

namespace dsl = lexy::dsl;
using node_vector = std::vector<node>;

// operator tags
struct add
{
};

struct sub
{
};

struct mul
{
};

struct div
{
};

struct pow
{
};

struct lt
{
};

struct gt
{
};

struct neg
{
};

template<typename Tag>
constexpr auto get_node_type() -> node_type
{
  if constexpr (std::is_same_v<Tag, add>) {
    return node_type::add;
  } else if constexpr (std::is_same_v<Tag, sub>) {
    return node_type::sub;
  } else if constexpr (std::is_same_v<Tag, mul>) {
    return node_type::mul;
  } else if constexpr (std::is_same_v<Tag, div>) {
    return node_type::div;
  } else if constexpr (std::is_same_v<Tag, pow>) {
    return node_type::pow;
  } else if constexpr (std::is_same_v<Tag, lt>) {
    return node_type::lt;
  } else if constexpr (std::is_same_v<Tag, gt>) {
    return node_type::gt;
  } else {
    static_assert(!sizeof(Tag), "Unknown operator tag");
  }
}

// Merge helpers — reverse-prefix encoding
//   Binary:  rhs..., lhs..., op
//   Unary:   child..., op
//   Ternary: else..., then..., cond..., op
inline auto merge_binary(node_type type, node_vector lhs, node_vector rhs)
    -> node_vector
{
  rhs.insert(rhs.end(),
             std::make_move_iterator(lhs.begin()),
             std::make_move_iterator(lhs.end()));
  rhs.push_back(node::function(type, 2));
  return rhs;
}

inline auto merge_unary(node_type type, node_vector child) -> node_vector
{
  child.push_back(node::function(type, 1));
  return child;
}

inline auto merge_ternary(node_type type,
                          node_vector cond,
                          node_vector then_branch,
                          node_vector else_branch) -> node_vector
{
  else_branch.insert(else_branch.end(),
                     std::make_move_iterator(then_branch.begin()),
                     std::make_move_iterator(then_branch.end()));
  else_branch.insert(else_branch.end(),
                     std::make_move_iterator(cond.begin()),
                     std::make_move_iterator(cond.end()));
  else_branch.push_back(node::function(type, 3));
  return else_branch;
}

// symbol tables
constexpr auto unary_symbols = lexy::symbol_table<node_type>
    .map<LEXY_SYMBOL("abs")>(node_type::abs)
    .map<LEXY_SYMBOL("square")>(node_type::square)
    .map<LEXY_SYMBOL("exp")>(node_type::exp)
    .map<LEXY_SYMBOL("log")>(node_type::log)
    .map<LEXY_SYMBOL("log1p")>(node_type::log1p)
    .map<LEXY_SYMBOL("logabs")>(node_type::logabs)
    .map<LEXY_SYMBOL("sin")>(node_type::sin)
    .map<LEXY_SYMBOL("cos")>(node_type::cos)
    .map<LEXY_SYMBOL("tan")>(node_type::tan)
    .map<LEXY_SYMBOL("asin")>(node_type::asin)
    .map<LEXY_SYMBOL("acos")>(node_type::acos)
    .map<LEXY_SYMBOL("atan")>(node_type::atan)
    .map<LEXY_SYMBOL("sinh")>(node_type::sinh)
    .map<LEXY_SYMBOL("cosh")>(node_type::cosh)
    .map<LEXY_SYMBOL("tanh")>(node_type::tanh)
    .map<LEXY_SYMBOL("sqrt")>(node_type::sqrt)
    .map<LEXY_SYMBOL("sqrtabs")>(node_type::sqrtabs)
    .map<LEXY_SYMBOL("cbrt")>(node_type::cbrt)
    .map<LEXY_SYMBOL("erf")>(node_type::erf)
    .map<LEXY_SYMBOL("sigmoid")>(node_type::sigmoid)
    .map<LEXY_SYMBOL("softplus")>(node_type::softplus)
    .map<LEXY_SYMBOL("gauss")>(node_type::gauss);

constexpr auto binary_symbols = lexy::symbol_table<node_type>
    .map<LEXY_SYMBOL("pow")>(node_type::pow)
    .map<LEXY_SYMBOL("powabs")>(node_type::powabs)
    .map<LEXY_SYMBOL("aq")>(node_type::aq)
    .map<LEXY_SYMBOL("min")>(node_type::fmin)
    .map<LEXY_SYMBOL("max")>(node_type::fmax)
    .map<LEXY_SYMBOL("lt")>(node_type::lt)
    .map<LEXY_SYMBOL("gt")>(node_type::gt);

constexpr auto ternary_symbols =
    lexy::symbol_table<node_type>.map<LEXY_SYMBOL("if")>(node_type::ternary);

// identifier (shared by functions and variables)
constexpr auto identifier = dsl::identifier(dsl::ascii::alpha_underscore,
                                            dsl::ascii::alpha_digit_underscore);

// grammar productions
struct expr;  // forward declaration

struct number
{
  static constexpr auto rule = []() -> auto
  {
    auto integer = dsl::sign + dsl::digits<>;
    auto fraction = dsl::period >> dsl::digits<>;
    auto exponent =
        (dsl::lit_c<'e'> / dsl::lit_c<'E'>) >> (dsl::sign + dsl::digits<>);
    return dsl::peek(dsl::lit_c<'.'> / dsl::digit<>) >> dsl::capture(
               dsl::token(integer + dsl::opt(fraction) + dsl::opt(exponent)));
  }();

  static constexpr auto value = []() -> auto
  {
    return lexy::as_string<std::string>
        | lexy::callback<node_vector>(
               [](std::string&& s)
               {
                 double v {};
                 [[maybe_unused]] auto res =
                     fast_float::from_chars(s.data(), s.data() + s.size(), v);
                 return node_vector {node::constant(v)};
               });
  }();
};

struct variable
{
  static constexpr auto rule = identifier;

  static constexpr auto value = []() -> auto
  {
    return lexy::as_string<std::string>
        | lexy::callback<node_vector>(
               [](std::string&& name)
               { return node_vector {node::variable(std::move(name))}; });
  }();
};

// unary function: name(expr)
struct unary_function
{
  static constexpr auto rule = []()
  {
    return dsl::symbol<unary_symbols>(identifier)
        >> dsl::parenthesized(dsl::recurse<expr>);
  }();

  static constexpr auto value = lexy::callback<node_vector>(
      [](node_type type, node_vector child) -> node_vector
      { return merge_unary(type, std::move(child)); });
};

// binary function: name(expr, expr)
struct binary_function
{
  static constexpr auto rule = []()
  {
    return dsl::symbol<binary_symbols>(identifier) >> dsl::parenthesized(
               dsl::recurse<expr> + dsl::comma + dsl::recurse<expr>);
  }();

  static constexpr auto value = lexy::callback<node_vector>(
      [](node_type type, node_vector lhs, node_vector rhs) -> node_vector
      { return merge_binary(type, std::move(lhs), std::move(rhs)); });
};

// ternary function: if(cond, then, else)
struct ternary_function
{
  static constexpr auto rule = []()
  {
    return dsl::symbol<ternary_symbols>(identifier) >> dsl::parenthesized(
               dsl::recurse<expr> + dsl::comma + dsl::recurse<expr>
               + dsl::comma + dsl::recurse<expr>);
  }();

  static constexpr auto value = lexy::callback<node_vector>(
      [](node_type type,
         node_vector cond,
         node_vector then_branch,
         node_vector else_branch) -> node_vector
      {
        return merge_ternary(type,
                             std::move(cond),
                             std::move(then_branch),
                             std::move(else_branch));
      });
};

// expression: atoms combined with operators, respecting precedence and
// associativity
struct expr : lexy::expression_production
{
  static constexpr auto whitespace = dsl::ascii::space;

  // Atom: the indivisible operand
  static constexpr auto atom = []() -> auto
  {
    return dsl::parenthesized(dsl::recurse<expr>)
        | dsl::p<ternary_function> | dsl::p<binary_function>
        | dsl::p<unary_function> | dsl::p<number> | dsl::p<variable>;
  }();

  // Operator groups — highest to lowest precedence

  struct prefix_neg : dsl::prefix_op
  {
    static constexpr auto op = dsl::op<neg>(dsl::lit_c<'-'>);
    using operand = dsl::atom;
  };

  struct power : dsl::infix_op_right
  {
    static constexpr auto op = dsl::op<pow>(dsl::lit_c<'^'>);
    using operand = prefix_neg;
  };

  struct multiplication : dsl::infix_op_left
  {
    static constexpr auto op =
        dsl::op<mul>(dsl::lit_c<'*'>) / dsl::op<div>(dsl::lit_c<'/'>);
    using operand = power;
  };

  struct addition : dsl::infix_op_left
  {
    static constexpr auto op =
        dsl::op<add>(dsl::lit_c<'+'>) / dsl::op<sub>(dsl::lit_c<'-'>);
    using operand = multiplication;
  };

  struct comparison : dsl::infix_op_left
  {
    static constexpr auto op =
        dsl::op<lt>(dsl::lit_c<'<'>) / dsl::op<gt>(dsl::lit_c<'>'>);
    using operand = addition;
  };

  using operation = comparison;

  // Callbacks
  static constexpr auto value = lexy::callback<node_vector>(
      // Atom passthrough
      [](node_vector nodes) -> node_vector { return nodes; },

      // All binary infix operators (generic)
      []<typename Tag>(node_vector lhs, Tag, node_vector rhs) -> auto
      {
        return merge_binary(
            get_node_type<Tag>(), std::move(lhs), std::move(rhs));
      },

      // Prefix negation → unary Sub
      [](neg, node_vector child) -> node_vector
      { return merge_unary(node_type::sub, std::move(child)); });
};

// top-level input
struct input
{
  static constexpr auto whitespace = dsl::ascii::space;
  static constexpr auto rule = dsl::p<expr> + dsl::eof;
  static constexpr auto value = lexy::forward<node_vector>;
};

}  // anonymous namespace

auto parse(std::string_view infix) -> std::variant<expression, parse_error>
{
  auto lexy_input =
      lexy::string_input<lexy::utf8_encoding>(infix.data(), infix.size());

  // Collect errors into a string
  std::string error_msg;
  auto error_callback =
      lexy_ext::report_error.to(std::back_insert_iterator(error_msg));

  auto result = lexy::parse<input>(lexy_input, error_callback);

  if (!result.has_value() || !error_msg.empty()) {
    return parse_error {.message = std::move(error_msg), .position = 0};
  }

  return std::move(result).value();
}

}  // namespace infix_parser

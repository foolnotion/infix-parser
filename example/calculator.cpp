// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Bogdan Burlacu <bogdan.burlacu@pm.me>

#include <cctype>
#include <cmath>
#include <iostream>
#include <map>
#include <numbers>
#include <stack>
#include <string>
#include <string_view>
#include <variant>

#include <fmt/core.h>

#include "infix-parser/ast.hpp"
#include "infix-parser/parser.hpp"

namespace
{

using infix_parser::expression;
using infix_parser::node_type;

auto evaluate(expression const& expr, std::map<std::string, double> const& vars)
    -> std::variant<double, std::string>
{
  std::stack<double> stk;

  auto pop = [&]
  {
    auto v = stk.top();
    stk.pop();
    return v;
  };

  for (auto const& n : expr) {
    switch (n.type) {
      case node_type::constant:
        stk.push(n.value);
        break;

      case node_type::variable: {
        auto it = vars.find(n.name);
        if (it == vars.end()) {
          return "undefined variable '" + n.name + "'";
        }
        stk.push(it->second);
        break;
      }

      // Binary arithmetic — stack order: lhs on top, rhs below
      case node_type::add: {
        auto lhs = pop(), rhs = pop();
        stk.push(lhs + rhs);
        break;
      }
      case node_type::sub: {
        if (n.arity == 1) {
          stk.push(-pop());
          break;
        }
        auto lhs = pop(), rhs = pop();
        stk.push(lhs - rhs);
        break;
      }
      case node_type::mul: {
        auto lhs = pop(), rhs = pop();
        stk.push(lhs * rhs);
        break;
      }
      case node_type::div: {
        auto lhs = pop(), rhs = pop();
        stk.push(lhs / rhs);
        break;
      }
      case node_type::pow: {
        auto lhs = pop(), rhs = pop();
        stk.push(std::pow(lhs, rhs));
        break;
      }

      // Comparisons → 0.0 / 1.0
      case node_type::lt: {
        auto lhs = pop(), rhs = pop();
        stk.push(lhs < rhs ? 1.0 : 0.0);
        break;
      }
      case node_type::gt: {
        auto lhs = pop(), rhs = pop();
        stk.push(lhs > rhs ? 1.0 : 0.0);
        break;
      }

      // Unary functions
      case node_type::abs:
        stk.push(std::abs(pop()));
        break;
      case node_type::square: {
        auto x = pop();
        stk.push(x * x);
        break;
      }
      case node_type::exp:
        stk.push(std::exp(pop()));
        break;
      case node_type::log:
        stk.push(std::log(pop()));
        break;
      case node_type::sin:
        stk.push(std::sin(pop()));
        break;
      case node_type::cos:
        stk.push(std::cos(pop()));
        break;
      case node_type::tan:
        stk.push(std::tan(pop()));
        break;
      case node_type::asin:
        stk.push(std::asin(pop()));
        break;
      case node_type::acos:
        stk.push(std::acos(pop()));
        break;
      case node_type::atan:
        stk.push(std::atan(pop()));
        break;
      case node_type::sinh:
        stk.push(std::sinh(pop()));
        break;
      case node_type::cosh:
        stk.push(std::cosh(pop()));
        break;
      case node_type::tanh:
        stk.push(std::tanh(pop()));
        break;
      case node_type::sqrt:
        stk.push(std::sqrt(pop()));
        break;
      case node_type::cbrt:
        stk.push(std::cbrt(pop()));
        break;
      case node_type::erf:
        stk.push(std::erf(pop()));
        break;
      case node_type::sigmoid: {
        auto x = pop();
        stk.push(1.0 / (1.0 + std::exp(-x)));
        break;
      }
      case node_type::softplus: {
        auto x = pop();
        stk.push(std::log1p(std::exp(x)));
        break;
      }
      case node_type::gauss: {
        auto x = pop();
        stk.push(std::exp(-x * x));
        break;
      }
      case node_type::log1p:
        stk.push(std::log1p(std::abs(pop())));
        break;
      case node_type::logabs:
        stk.push(std::log(std::abs(pop())));
        break;
      case node_type::sqrtabs:
        stk.push(std::sqrt(std::abs(pop())));
        break;

      // Binary functions
      case node_type::aq: {
        auto lhs = pop(), rhs = pop();
        stk.push(lhs / std::sqrt(1.0 + rhs * rhs));
        break;
      }
      case node_type::fmin: {
        auto lhs = pop(), rhs = pop();
        stk.push(std::fmin(lhs, rhs));
        break;
      }
      case node_type::fmax: {
        auto lhs = pop(), rhs = pop();
        stk.push(std::fmax(lhs, rhs));
        break;
      }
      case node_type::powabs: {
        auto lhs = pop(), rhs = pop();
        stk.push(std::pow(std::abs(lhs), rhs));
        break;
      }

      // Ternary: smooth blend  cond*then + (1-cond)*else
      case node_type::ternary: {
        auto cond = pop(), then_val = pop(), else_val = pop();
        stk.push(cond * then_val + (1.0 - cond) * else_val);
        break;
      }

      default:
        return std::string {"unsupported node type"};
    }
  }

  if (stk.size() != 1) {
    return std::string {"evaluation error: malformed expression"};
  }
  return stk.top();
}

auto trim(std::string_view s) -> std::string_view
{
  auto a = s.find_first_not_of(" \t\r\n");
  if (a == std::string_view::npos) {
    return {};
  }
  return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

}  // namespace

int main()
{
  std::map<std::string, double> vars = {
      {"pi", std::numbers::pi},
      {"e", std::numbers::e},
      {"tau", std::numbers::pi + std::numbers::pi},
      {"phi", std::numbers::phi},
  };

  fmt::println("infix calculator  (operators: + - * / ^ < >)");
  fmt::println("functions: sin cos tan asin acos atan sinh cosh tanh");
  fmt::println("           exp log sqrt cbrt abs square pow min max aq");
  fmt::println("           sigmoid softplus gauss  |  if(cond,then,else)");
  fmt::println("constants: pi  e  tau  phi");
  fmt::println("type 'vars' to list variables, 'quit' to exit\n");

  std::string line;
  while (true) {
    std::cout << "> " << std::flush;

    if (!std::getline(std::cin, line)) {
      fmt::println("");
      break;
    }

    auto trimmed = std::string {trim(line)};
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed == "quit" || trimmed == "exit") {
      break;
    }

    if (trimmed == "vars") {
      for (auto const& [name, val] : vars) {
        fmt::println("  {} = {}", name, val);
      }
      continue;
    }

    // Detect "identifier = expr" assignment
    std::string var_name;
    std::string_view expr_sv = trimmed;

    auto eq = trimmed.find('=');
    if (eq != std::string::npos && eq > 0) {
      auto name_sv = trim(trimmed.substr(0, eq));
      bool valid = !name_sv.empty()
          && (std::isalpha(static_cast<unsigned char>(name_sv[0]))
              || name_sv[0] == '_');
      for (char c : name_sv) {
        valid =
            valid && (std::isalnum(static_cast<unsigned char>(c)) || c == '_');
      }
      if (valid) {
        var_name = std::string {name_sv};
        expr_sv = trim(trimmed.substr(eq + 1));
      }
    }

    auto parse_result = infix_parser::parse(expr_sv);
    if (auto* err = std::get_if<infix_parser::parse_error>(&parse_result)) {
      fmt::println(stderr, "parse error: {}", err->message);
      continue;
    }

    auto& expr = std::get<infix_parser::expression>(parse_result);
    auto eval_result = evaluate(expr, vars);

    if (auto* err = std::get_if<std::string>(&eval_result)) {
      fmt::println(stderr, "error: {}", *err);
      continue;
    }

    double value = std::get<double>(eval_result);
    if (!var_name.empty()) {
      vars[var_name] = value;
      fmt::println("{} = {}", var_name, value);
    } else {
      fmt::println("{}", value);
    }
  }

  return 0;
}

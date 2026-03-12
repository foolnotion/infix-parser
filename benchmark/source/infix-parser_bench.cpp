// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Bogdan Burlacu <bogdan.burlacu@pm.me>

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>
#include <variant>

#include <nanobench.h>

#include "infix-parser/parser.hpp"

namespace nb = ankerl::nanobench;

using case_entry = std::pair<std::string_view, std::string_view>;
using case_array = std::array<case_entry, 12>;

// Expressions ordered roughly by AST size.
// Node counts are verified at startup against the actual parse result.
static constexpr case_array cases = {{
    // trivial
    {"constant", "3.14159265358979"},
    {"variable", "x1"},
    // single operator / function
    {"add", "x1 + x2"},
    {"unary-sin", "sin(x1)"},
    // small compound
    {"pythagorean", "sin(x1)^2 + cos(x1)^2"},
    {"affine", "x1 * x2 + x3 * x4 + x5"},
    // medium — representative symbolic-regression building blocks
    {"trig-product", "sin(x1) * cos(x2) + exp(x3)"},
    {"poly4", "x1^4 + x2^3 + x3^2 + x4"},
    {"norm2", "sqrt(x1^2 + x2^2 + x3^2)"},
    {"koza1", "x1^4 + x1^3 + x1^2 + x1"},
    // larger — deeper nesting, more distinct functions
    {"composite-large",
     "sin(x1*x2)/(abs(x3)+exp(-x4^2))+log(abs(x5))*sqrt(x6^2+x7^2)"},
    {"deep-nested", "sin(cos(tan(abs(log(sqrt(x1+x2))))))"},
}};

static constexpr auto warmup_iterations = 1'000U;
static constexpr auto min_epoch_iterations = 10'000U;

int main(int argc, char** argv)
{
  // Optional: write nanobench JSON to a file for CI tracking.
  // Usage:  infix-parser_bench results.json
  bool const write_json = (argc == 2);

  nb::Bench bench;
  bench.title("infix-parser")
      .unit("node")
      .warmup(warmup_iterations)
      .minEpochIterations(min_epoch_iterations)
      .relative(true);

  for (auto const& [name, expr] : cases) {
    auto first = infix_parser::parse(expr);
    if (!std::holds_alternative<infix_parser::expression>(first)) {
      std::cerr << "SETUP ERROR: failed to parse '" << expr << "'\n";
      return EXIT_FAILURE;
    }
    auto node_count = std::get<infix_parser::expression>(first).size();

    bench.batch(static_cast<double>(node_count))
        .run(std::string {name},
             [&]
             {
               auto result = infix_parser::parse(expr);
               nb::doNotOptimizeAway(result);
             });
  }

  if (write_json) {
    std::ofstream out {argv[1]};
    if (!out) {
      std::cerr << "error: could not open '" << argv[1] << "' for writing\n";
      return EXIT_FAILURE;
    }
    bench.render(nb::templates::json(), out);
  }

  return EXIT_SUCCESS;
}

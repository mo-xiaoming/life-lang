// Integration test for README example code
// Ensures the example in README.md can be successfully parsed
//
// *** IMPORTANT: KEEP THIS IN SYNC WITH README.md ***
// This test contains the EXACT code from the README.md "Example Code" section.
// When updating README.md example, update this test file accordingly.
// When this test fails, it means we need to add parser support for the README example.

#include <doctest/doctest.h>
#include <iostream>

#include "diagnostics.hpp"
#include "parser.hpp"

TEST_CASE("Parse README example code") {
  // *** EXACT COPY FROM README.md - DO NOT MODIFY WITHOUT UPDATING README ***
  auto const* const k_readme_example = R"life_code(
// Generic Result type with struct variants
enum Result<T, E> {
    Ok(T),
    Err(E),
}

// Trait with generic bounds and where clause
trait Processor<T>
where
    T: Display + Clone
{
    fn process(self, item: T): Result<T, String>;
}

// Struct with impl block
struct Point {
    x: I32,
    y: I32,
}

impl Point {
    fn distance(self): F64 {
        let dx = self.x * self.x;
        let dy = self.y * self.y;
        return Std.Math.sqrt((dx + dy).into());
    }
}

// Trait implementation
impl Display for Point {
    fn to_string(self): String {
        return Std.Format.format("({}, {})", self.x, self.y);
    }
}

// Pattern matching with if-expression
fn process_result<T: Display>(result: Result<T, String>): I32 {
    return match result {
        Result.Ok(value) => {
            Std.IO.println("Success: " + value.to_string());
            0
        },
        Result.Err(msg) => {
            Std.IO.println("Error: " + msg);
            1
        },
    };
}

fn main(args: Array<String>): I32 {
    let point = Point { x: 3, y: 4 };
    let dist = point.distance();

    let result = if dist > 5.0 {
        Result.Ok(point)
    } else {
        Result.Err("Too close")
    };

    return process_result(result);
}
)life_code";

  life_lang::Diagnostic_Engine diagnostics{"readme_example.life", k_readme_example};
  life_lang::parser::Parser parser{diagnostics};
  auto const result = parser.parse_module();

  // If parsing fails, print diagnostics for debugging
  if (!result) {
    INFO("Parse failed with diagnostics:");
    diagnostics.print(std::cerr);
  }

  // Verify successful parse
  REQUIRE(result.has_value());
}

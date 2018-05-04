#include <catch.hpp>

#include "include/test_helpers.hpp"

TEST_CASE("while loops should loop", "[control]") {
    SECTION( "basic while loop" ) {
        auto code = build_string(R"(
x = 0
while x < 100:
    x += 1
check_int(x)
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int"] = make_builtin_check_value((int64_t)100);
        state.eval();
    }
}
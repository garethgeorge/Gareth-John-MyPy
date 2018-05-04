#include "../lib/catch.hpp"

#include "include/test_helpers.hpp"
#include "../src/builtins/builtins.hpp"

TEST_CASE("lists should work", "[lists]") {

    SECTION("can create a list") {
        auto code = build_string(R"(
[1,2,3,4,5]
        )");
        InterpreterState state(code);
        state.eval();
    }

    SECTION("can access a value in a list") {
        auto code = build_string(R"(
check_val([1,2,3,4,5][4])
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_val"] = make_builtin_check_value((int64_t)5);
        state.eval();
    }

    SECTION("can access a value in a list") {
        auto code = build_string(R"(
check_val([1,2,3,4,5][-1])
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_val"] = make_builtin_check_value((int64_t)5);
        state.eval();
    }

    SECTION("can loop through a list") {
        auto code = build_string(R"(
myList = [1, 2, 3, 4, 5]
x = 0
while x < len(myList):
  check_val(myList[x] == x + 1)
  x += 1
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val"] = make_builtin_check_value((bool)true);
        state.eval();
    }

}
#include <catch.hpp>

#include "include/test_helpers.hpp"

TEST_CASE("should be able to perform arithmetic", "[arithmetic]") {
    SECTION( "adding two values" ) {
        auto code = build_string(R"(
x = 5
check_int(x + 10)
check_double(x + 10.5)
        )");
        InterpreterState state(code);
        state.ns_builtins["check_int"] = make_builtin_check_value((int64_t)15);
        state.ns_builtins["check_double"] = make_builtin_check_value((double)15.5);
        state.eval();
    }

    SECTION( "multiply two values" ) {
        const char *theCode = R"(
x = 5
check_int(x * 5)
check_double(x * 5.5)
check_double(5.5 * x)
)";
        auto code = build_string(theCode);
        InterpreterState state(code);
        state.ns_builtins["check_int"] = make_builtin_check_value((int64_t)25);
        state.ns_builtins["check_double"] = make_builtin_check_value((double)27.5);
        state.eval();
    }

    SECTION( "subtract two values" ) {
        const char *theCode = R"(
x = 5
check_int(x - 5)
)";
        auto code = build_string(theCode);
        InterpreterState state(code);
        state.ns_builtins["check_int"] = make_builtin_check_value((int64_t)0);
        state.eval();
    }

    SECTION( "add two strings" ) {
        const char *theCode = R"(
x = "test"
check_string(x + " test")
)";
        auto code = build_string(theCode);
        InterpreterState state(code);
        state.ns_builtins["check_string"] = make_builtin_check_value(make_shared<string>("test test"));
        state.eval();
    }

    SECTION( "can not multiply an int and a string" ) {
        const char *theCode = R"(
x = 5
x * "test"
)";
        auto code = build_string(theCode);
        InterpreterState state(code);
        state.ns_builtins["check_int"] = make_builtin_check_value((int64_t)25);
        state.ns_builtins["check_double"] = make_builtin_check_value((double)27.5);
        REQUIRE_THROWS(state.eval());
    }
}
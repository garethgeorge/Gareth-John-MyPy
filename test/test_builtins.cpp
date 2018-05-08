#include <catch.hpp>

#include "include/test_helpers.hpp"
#include "../src/builtins/builtins.hpp"

#include <iostream>

TEST_CASE("should be able to call a builtin", "[builtins]") {

    SECTION("wat is up") {

        auto myFunc = builtins::make_cfunction([]() {
            std::cout << "hello world!";
            return;
        });

        const char *theCode = R"(
x = 5
check_int()
)";
        auto code = build_string(theCode);
        InterpreterState state(code);
        state.ns_builtins["check_int"] = std::make_shared<value::CFunction>(myFunc.wrap_in_function());
        state.eval();

    }

}
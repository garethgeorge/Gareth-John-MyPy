#include <catch.hpp>

#include "include/test_helpers.hpp"
#include "../src/builtins/builtins.hpp"
#include "../src/pyvalue.hpp"

#include <iostream>

TEST_CASE("list builtins", "[builtins]") {
    SECTION( "should be able to appned to a list" ) {
        auto code = build_string(R"(
x = []
x.append(15)
check_int(x[0])
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int"] = make_builtin_check_value((int64_t)15);
        state.eval();
    }
}

// TEST_CASE("should be able to call a builtin", "[builtins]") {
//     SECTION("my dumb builtin test") {
//         auto myFunc = builtins::pycfunction_builder([](int64_t a, int64_t b) {
//             return a + b;
//         }).to_pycfunction();

//         auto myFuncPrint = builtins::pycfunction_builder::fromCallable([](int64_t val) {
//             std::cout << "value: " << val << std::endl;
//         })

//         const char *theCode = R"(
// check_int(15, 20)
// )";
//         auto code = build_string(theCode);
//         InterpreterState state(code);
//         state.ns_builtins["check_int"] = myFunc;
//         state.ns_builtins["print"] = myFuncPrint;
//         state.eval();

//     }

// }
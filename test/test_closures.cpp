#include "../lib/catch.hpp"
#include "../src/builtins/builtins.hpp"
#include "../src/builtins/builtins_helpers.hpp"

#include "include/test_helpers.hpp"

using namespace builtins;

TEST_CASE("Closures should work", "[closures]") {
    SECTION( "in a basic case" ){
        auto code = build_string(R"(
def print_msg(msg):
    def printer():
        return msg
    return printer

a = print_msg(100)
/*check_int1(a())
b = print_msg(200)
check_int2(b())
check_int3(a())
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)8);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)8);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)8);
        state.eval();
    }
}
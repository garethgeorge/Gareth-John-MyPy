#include "../lib/catch.hpp"
#include "../src/builtins/builtins.hpp"

#include "include/test_helpers.hpp"

using namespace builtins;

TEST_CASE("should be able to call a function", "[functions]") {
    SECTION( "without default args" ){
        auto code = build_string(R"(
def test_func(a,b):
    return a + b

check_int(test_func(3,5))
check_int2(test_func(5,3))
check_double(test_func(5.0,3.2))
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int"] = make_builtin_check_value((int64_t)8);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)8);
        (*(state.ns_builtins))["check_double"] = make_builtin_check_value((double)8.2);
        state.eval();
    }
    SECTION( "with default args" ){
        auto code = build_string(R"(
def test_func(a=3,b=5):
    return a + b

check_int(test_func())
check_int2(test_func(1,2))
check_double(test_func(5.0))
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int"] = make_builtin_check_value((int64_t)8);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_double"] = make_builtin_check_value((double)10.0);
        state.eval();
    }
    SECTION( "with some default args" ){
        auto code = build_string(R"(
def test_func(a,b=5):
    return a + b

check_int(test_func(7))
check_int2(test_func(1,2))
check_double(test_func(5.0))
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int"] = make_builtin_check_value((int64_t)12);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_double"] = make_builtin_check_value((double)10.0);
        state.eval();        
    }
    SECTION( "from another function" ){
        auto code = build_string(R"(
def func_a(a):
    return a + 1

def func_b(b):
    return b + func_a(b)

check_int(func_b(2))
check_int2(func_b(100))
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)201);
        state.eval();       
    }
    SECTION( "recursively" ){
        auto code = build_string(R"(
def func_r(a):
    if(a == 0):
        return 0
    else:
        return a + func_r(a - 1)

check_int(func_r(2))
check_int2(func_r(20))
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)210);
        state.eval();       
    }
}

TEST_CASE("should cause an error when incorrectly calling a function", "[functions]") {
    SECTION( "when not passed enough args"){
        auto code = build_string(R"(
def func_err(a,b,c=5):
    return a + b + c

func_err(1)
        )");
        InterpreterState state(code);
        REQUIRE_THROWS(state.eval());
    }

    SECTION( "when passed too many args"){
        auto code = build_string(R"(
def func_err(a,b,c=5):
    return a + b + c

func_err(1,2,3,4)
        )");
        InterpreterState state(code);
        REQUIRE_THROWS(state.eval());
    }

    SECTION( "functions that contain while loops (more complex degrees of nesting etc) work" ) {
        // this code was causing an error with STORE_FAST, I changed STORE_FAST to use co_varnames 
        // instead of co_names as per the spec. LOAD_FAST was already using co_varnames 
        // as is correct.
        auto code = build_string(R"(
def range222():
    x = 1
    while x < 10:
        print(x)
        x += 1
generator = range222()
        )");

        InterpreterState state(code);
        (*(state.ns_builtins))["print"] = pycfunction_builder([] (Value value) {
            // pass, but it is necessary that the function exist
        }).to_pycfunction();
        state.eval();
    }

    SECTION("should get arguments in the correct order") {
        auto code = build_string(R"(
def test(a, b, c):
    check(a == 1)
    check(b == 2)
    check(c == 3)
test(1, 2, 3)
        )");

        InterpreterState state(code);
        (*(state.ns_builtins))["check"] = make_builtin_check_value(true);
        state.eval();       

    }

}


TEST_CASE("should be able to use a generator function", "[functions]") {
    SECTION("simple range generator should work") {
        auto code = build_string(R"(
def range(n):
    x = 0
    while x < n:
        yield x 
        x += 1 
y = 0
for x in range(10):
    y += x
check_value(y)
        )");

        InterpreterState state(code);
        (*(state.ns_builtins))["check_value"] = make_builtin_check_value((int64_t)(1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9));
        state.eval();
    }

    SECTION("complex generator comprehensions should also work") {
        auto code = build_string(R"(
def range(n):
    x = 0
    while x < n:
        yield x 
        x += 1 
y = 0
for x in (x * x for x in (x + 1 for x in range(10) if x != 5)):
    y += x
check_value(349)
        )");

        InterpreterState state(code);
        (*(state.ns_builtins))["check_value"] = make_builtin_check_value((int64_t)(349));
        state.eval();
    }
}
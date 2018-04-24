#include "../lib/catch.hpp"

#include "include/test_helpers.hpp"

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
        state.ns_builtins["check_int"] = make_builtin_check_value((int64_t)8);
        state.ns_builtins["check_int2"] = make_builtin_check_value((int64_t)8);
        state.ns_builtins["check_double"] = make_builtin_check_value((double)8.2);
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
        state.ns_builtins["check_int"] = make_builtin_check_value((int64_t)8);
        state.ns_builtins["check_int2"] = make_builtin_check_value((int64_t)3);
        state.ns_builtins["check_double"] = make_builtin_check_value((double)10.0);
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
        state.ns_builtins["check_int"] = make_builtin_check_value((int64_t)12);
        state.ns_builtins["check_int2"] = make_builtin_check_value((int64_t)3);
        state.ns_builtins["check_double"] = make_builtin_check_value((double)10.0);
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
        state.ns_builtins["check_int"] = make_builtin_check_value((int64_t)5);
        state.ns_builtins["check_int2"] = make_builtin_check_value((int64_t)201);
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
        state.ns_builtins["check_int"] = make_builtin_check_value((int64_t)3);
        state.ns_builtins["check_int2"] = make_builtin_check_value((int64_t)210);
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

    SECTION( "passed too many args"){
        auto code = build_string(R"(
def func_err(a,b,c=5):
    return a + b + c

func_err(1,2,3,4)
        )");
        InterpreterState state(code);
        REQUIRE_THROWS(state.eval());
    }
}
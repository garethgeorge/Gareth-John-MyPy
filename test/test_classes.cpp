#include "../lib/catch.hpp"
#include "../src/builtins/builtins.hpp"
#include "include/test_helpers.hpp"

TEST_CASE("should be able to use classes", "[classes]") {
    SECTION( "and the functions in them" ){
        auto code = build_string(R"(
class simple_class:
    val = 4
    buf = 6
    def show(selfa,y):
        return(selfa.val + y)
    def __init__(self,r=6):
        self.val = r

bar = simple_class(2)
boo = simple_class(9)
check_int(boo.show(bar.show(3)))
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int"] = make_builtin_check_value((int64_t)14);
        state.eval();
    }

    SECTION( "and the values in them" ){
        auto code = build_string(R"(
class simple_class:
    val = 4
    buf = 6
    def show(selfa,y):
        return(selfa.val + y)
    def __init__(self,r=6):
        self.val = r

bar = simple_class(2)
boo = simple_class(9)
check_int1(bar.val)
check_int2(simple_class.val)
check_int3(bar.buf)
check_int4(simple_class.buf)
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)4);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)6);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)6);
        state.eval();
    }

    SECTION( "and set values later" ){
        auto code = build_string(R"(
class simple_class:
    val = 4
    buf = 6
    def show(selfa,y):
        return(selfa.val + y)
    def __init__(self,r=6):
        self.val = r

bar = simple_class(2)
boo = simple_class(9)
simple_class.val = 100
simple_class.buf = 101
check_int1(bar.show(10))
check_int2(bar.val);
check_int3(simple_class.val)
check_int4(bar.buf)
check_int5(simple_class.buf)
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)12);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)100);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)101);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)101);
        state.eval();
    }

    SECTION( "and set values later, 2" ){
        auto code = build_string(R"(
class simple_class:
    val = 4
    buf = 6
    def show(selfa,y):
        return(selfa.val + y)
    def __init__(self,r=6):
        self.val = r

bar = simple_class(2)
boo = simple_class(9)
simple_class.val = 100
simple_class.buf = 101
bar.buf = -100;
check_int1(bar.val);
check_int2(simple_class.val)
check_int3(bar.buf)
check_int4(simple_class.buf)
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)100);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)-100);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)101);
        state.eval();
    }

    SECTION( "and add fields later" ){
        auto code = build_string(R"(
class simple_class:
    val = 4
    buf = 6
    def show(selfa,y):
        return(selfa.val + y)
    def __init__(self,r=6):
        self.val = r

bar = simple_class(2)
boo = simple_class(9)
bar.uhoh = 5
check_int1(bar.uhoh)
simple_class.wow = 10
check_int2(simple_class.wow)
check_int3(bar.wow)
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)10);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)10);
        state.eval();
    }
}
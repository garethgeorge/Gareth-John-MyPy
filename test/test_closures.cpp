#include "../lib/catch.hpp"
#include "../src/builtins/builtins.hpp"
#include "../src/builtins/builtins_helpers.hpp"

#include "include/test_helpers.hpp"

using namespace builtins;

TEST_CASE("Closures should work", "[closures]") {
    SECTION( "in a basic case" ){
        std::cout << "START LOOKING HERE, HERE IS A SPECIAL SEQUENCE TO SEARCH FOR: 1aenifpnneivafp" << std::endl;
        auto code = build_string(R"(
def print_msg(msg):
    def printer():
        return msg
    return printer

a = print_msg(100)
check_int1(a())
b = print_msg(200)
check_int2(b())
check_int3(a())
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)100);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)200);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)100);
        state.eval();

        std::cout << "STOP LOOKING HERE, THIS IS THAT SEQUENCE AGAIN: 1aenifpnneivafp" << std::endl;
    }

    SECTION( "in a nested case" ){
        auto code = build_string(R"(
def ret_msg(msg):
    def printer(alt):
        def b():
            return(msg + 4)
        return b
    return printer(msg)

c = ret_msg(1)
check_int1(c())
d = ret_msg(5.5)
check_int2(d())
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((double)9.5);
        state.eval();
    }

    SECTION( "and can have modified inputs" ){
        auto code = build_string(R"(
def print_msg_store(msg):
    msg = -1
    def printer():
        return msg
    return printer

e = print_msg_store(300)
check_int1(e())
        )");
        InterpreterState state(code);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)-1);
        state.eval();
    }
}
    /*SECTION( "and correct changes as classes do or dont" ){
        auto code = build_string(R"(
weird = 10
class Ah:
    val = weird
    def getfunc(self,something):
        def bar():
            return(self.val + something)
        return bar
    )");*/
/*
f = Ah()
g = f.getfunc(10)
check_int1(g())
f.val = 12
check_int2(g())
h = Ah()
i = h.getfunc(30)
check_int3(i())
weird = 90
check_int4(i())
        )");*/
        //InterpreterState state(code);
        //(*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)-1);
        //(*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)-1);
        //(*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)-1);
        //(*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)-1);
       // state.eval();
    //}
//}
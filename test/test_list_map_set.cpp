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

    SECTION("Can slice values at the the start of a list"){
                auto code = build_string(R"(
values = [100, 200, 300, 400, 500,600,700,800,900,1000,1100,1200]
part = values[:3]
check_val1(len(part))
check_val2(part[0])
check_val3(part[1])
check_val4(part[2])
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val1"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_val2"] = make_builtin_check_value((int64_t)100);
        (*(state.ns_builtins))["check_val3"] = make_builtin_check_value((int64_t)200);
        (*(state.ns_builtins))["check_val4"] = make_builtin_check_value((int64_t)300);
        state.eval();
    }

    SECTION("Can return the same list after a slice"){
                auto code = build_string(R"(
values = [100, 200, 300, 400, 500,600,700,800,900,1000,1100,1200]
part = values[:2000]
check_val1(len(part))
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val1"] = make_builtin_check_value((int64_t)12);
        state.eval();
    }

    SECTION("Can slice until a point before the end of a list"){
                auto code = build_string(R"(
values = [1,2,3,4,5,6,7,8,9,10,11,12]
part = values[:-6]
check_val1(len(part))
check_val2(part[0])
check_val3(part[1])
check_val4(part[2])
check_val5(part[3])
check_val6(part[4])
check_val7(part[5])
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val1"] = make_builtin_check_value((int64_t)6);
        (*(state.ns_builtins))["check_val2"] = make_builtin_check_value((int64_t)1);
        (*(state.ns_builtins))["check_val3"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_val4"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_val5"] = make_builtin_check_value((int64_t)4);
        (*(state.ns_builtins))["check_val6"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_val7"] = make_builtin_check_value((int64_t)6);
        state.eval();
    }

    SECTION("Can slice from a starting point"){
                auto code = build_string(R"(
values = [1,2,3,4,5,6,7,8,9,10,11,12]
part = values[7:]
check_val1(len(part))
check_val2(part[0])
check_val3(part[1])
check_val4(part[2])
check_val5(part[3])
check_val6(part[4])
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val1"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_val2"] = make_builtin_check_value((int64_t)8);
        (*(state.ns_builtins))["check_val3"] = make_builtin_check_value((int64_t)9);
        (*(state.ns_builtins))["check_val4"] = make_builtin_check_value((int64_t)10);
        (*(state.ns_builtins))["check_val5"] = make_builtin_check_value((int64_t)11);
        (*(state.ns_builtins))["check_val6"] = make_builtin_check_value((int64_t)12);
        state.eval();
    }

    SECTION("Can slice out a part of the middle of a list"){
                auto code = build_string(R"(
values = [1,2,3,4,5,6,7,8,9,10,11,12]
part = values[2:4]
check_val1(len(part))
check_val2(part[0])
check_val3(part[1])
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_val2"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_val3"] = make_builtin_check_value((int64_t)4);
        state.eval();
    }

    SECTION("Can slice and give a step size"){
                auto code = build_string(R"(
values = [1,2,3,4,5,6,7,8,9,10,11,12]
part = values[0:4:3]
check_val1(len(part))
check_val2(part[0])
check_val3(part[1])
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_val2"] = make_builtin_check_value((int64_t)1);
        (*(state.ns_builtins))["check_val3"] = make_builtin_check_value((int64_t)4);
        state.eval();
    }

    SECTION("Can slice and give a step size (test two)"){
                auto code = build_string(R"(
values = [1,2,3,4,5,6,7,8,9,10,11,12]
part = values[1:-2:4]
check_val1(len(part))
check_val2(part[0])
check_val3(part[1])
check_val4(part[2])
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val1"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_val2"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_val3"] = make_builtin_check_value((int64_t)6);
        (*(state.ns_builtins))["check_val4"] = make_builtin_check_value((int64_t)10);
        state.eval();
    }

    SECTION("Can slice from a starting point and take steps"){
                auto code = build_string(R"(
values = [1,2,3,4,5,6,7,8,9,10,11,12]
part = values[7::2]
check_val1(len(part))
check_val2(part[0])
check_val3(part[1])
check_val4(part[2])
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val1"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_val2"] = make_builtin_check_value((int64_t)8);
        (*(state.ns_builtins))["check_val3"] = make_builtin_check_value((int64_t)10);
        (*(state.ns_builtins))["check_val4"] = make_builtin_check_value((int64_t)12);
        state.eval();
    }

    SECTION("Can slice out from a whole list but taking steps"){
                auto code = build_string(R"(
values = [1,2,3,4,5,6,7,8,9,10,11,12]
part = values[::3]
check_val1(len(part))
check_val2(part[0])
check_val3(part[1])
check_val4(part[2])
check_val5(part[3])
        )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_val1"] = make_builtin_check_value((int64_t)4);
        (*(state.ns_builtins))["check_val2"] = make_builtin_check_value((int64_t)1);
        (*(state.ns_builtins))["check_val3"] = make_builtin_check_value((int64_t)4);
        (*(state.ns_builtins))["check_val4"] = make_builtin_check_value((int64_t)7);
        (*(state.ns_builtins))["check_val5"] = make_builtin_check_value((int64_t)10);
        state.eval();
    }

}
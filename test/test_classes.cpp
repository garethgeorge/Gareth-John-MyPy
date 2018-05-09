#include "../lib/catch.hpp"
#include "../src/builtins/builtins.hpp"
#include "include/test_helpers.hpp"

TEST_CASE("Classes should work", "[classes]") {
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

        SECTION( "and use method type decoratiors" ){
        auto code = build_string(R"(
class A:
    val = 5

    def im(self):
        return(self.val)

    @classmethod
    def cm(self):
        return(self.val)

    @staticmethod
    def sm(self):
        return(self.val)

foo = A()
bar = A()
foo.val = 20
bar.val = 21

check_int1(foo.im())
check_int2(foo.cm())
check_int3(foo.sm(bar))
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)20);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)21);
        state.eval();
    }
}

TEST_CASE("Classes should inherit", "[classes]") {
    SECTION( "from their parent" ){
        auto code = build_string(R"(
class A:
    foo = 2
    bar = 5

class B(A):
    bar = 6

aaz = A()
baz = B()
check_int1(aaz.foo)
check_int2(aaz.bar)
check_int3(baz.foo)
check_int4(baz.bar)
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)6);
        state.eval();
    }

    SECTION( "from their parent's parent" ){
        auto code = build_string(R"(
class A:
    foo = 2
    bar = 5
    baz = 9

class B(A):
    bar = 6

class C(B):
    baz = 11

tester = C()
check_int1(tester.foo)
check_int2(tester.bar)
check_int3(tester.baz)
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)6);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)11);
        state.eval();
    }

    SECTION( "from their multiple parents" ){
        auto code = build_string(R"(
class A:
    foo = 2

class B:
    bar = 6

class C(A, B):
    baz = 11

tester = C()
check_int1(tester.foo)
check_int2(tester.bar)
check_int3(tester.baz)
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)6);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)11);
        state.eval();
    }

    SECTION( "changes to their parents" ){
        auto code = build_string(R"(
class A:
    foo = 2

class B(A):
    bar = 6

class C(B):
    pass

baz = C()
check_int1(baz.foo)
A.foo = 10
check_int2(baz.foo)
B.foo = 20
check_int3(baz.foo)
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)10);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)20);
        state.eval();
    }

    SECTION( "based on the order parents are listed" ){
        auto code = build_string(R"(
class A:
    foo = 2

class B:
    foo = 3

class C(A,B):
    pass

class D(B,A):
    pass

bar = C()
baz = D()
check_int1(bar.foo)
check_int2(baz.foo)
)");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)3);
        state.eval();
    }
}
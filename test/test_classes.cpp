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

TEST_CASE("Classes should allow operator overloading", "[classes]") {
    SECTION( "of the + operator"){
            auto code = build_string(R"(
class A:
    val = 1
    def __add__(self, other):
        self.val = self.val + other
        return self.val
    def __iadd__(self,other):
        self.val = self.val - other
        return self

class B:
    val = 2
    def __radd__(self,other):
        return self.val + other
    def __iadd__(self,other):
        self.val = self.val - other
        return self.val

foo = A()
bar = B()
check_int1(foo + 2)
check_int2(foo.val)
check_int3(3 + bar)
check_int4(bar.val)
check_int5(foo + bar)
check_int6(foo.val)
check_int7(bar.val)
foo += 3
check_int8(foo.val)
bar += 1000
check_int9(bar)             
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)-998);
        state.eval();
    }

    SECTION( "of the - operator"){
        auto code = build_string(R"(
class A:
    val = 1
    def __sub__(self, other):
        self.val = self.val - other
        return self.val
    def __isub__(self,other):
        self.val = self.val + other
        return self

class B:
    val = 2
    def __rsub__(self,other):
        return self.val - other
    def __isub__(self,other):
        return self.val + other

foo = A()
bar = B()
check_int1(foo - 2)
check_int2(foo.val)
check_int3(3 - bar)
check_int4(bar.val)
check_int5(foo - bar)
check_int6(foo.val)
check_int7(bar.val)
foo -= 3
check_int8(foo.val)
bar -= 1000
check_int9(bar) 
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)-1);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)-1);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)-1);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((int64_t)6);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)1002);
        state.eval();
    }

    SECTION( "of the * operator"){
        auto code = build_string(R"(
class A:
    val = 1
    def __mul__(self, other):
        self.val = self.val * other
        return self.val
    def __imul__(self,other):
        self.val = self.val * other
        return self

class B:
    val = 2
    def __rmul__(self,other):
        return self.val * other
    def __imul__(self,other):
        return self.val * other

foo = A()
bar = B()
check_int1(foo * 2)
check_int2(foo.val)
check_int3(3 * bar)
check_int4(bar.val)
check_int5(foo * bar)
check_int6(foo.val)
check_int7(bar.val)
foo *= 3
check_int8(foo.val)
bar *= 1000
check_int9(bar)
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)6);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)4);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((int64_t)4);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((int64_t)12);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)2000);
        state.eval();
    }

SECTION( "of the / operator"){
        auto code = build_string(R"(
class A:
    val = 1
    def __truediv__(self, other):
        self.val = self.val / other
        return self.val
    def __itruediv__(self,other):
        self.val = self.val / other
        return self

class B:
    val = 2
    def __rtruediv__(self,other):
        return self.val / other
    def __itruediv__(self,other):
        return self.val / other

foo = A()
bar = B()
check_int1(foo / 2)
check_int2(foo.val)
check_int3(4 / bar)
check_int4(bar.val)
check_int5(foo / bar)
check_int6(foo.val)
check_int7(bar.val)
foo /= 4
check_int8(foo.val)
bar /= 1000
check_int9(bar)
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        // Checking for equility with floats is okay here because the are all unambiguous
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((double)0.5);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((double)0.5);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((double)0.5);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((double)4.0);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((double)4.0);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((double)1.0);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((double)0.002);
        state.eval();
    }

    SECTION( "of the // operator"){
        auto code = build_string(R"(
class A:
    val = 10
    def __floordiv__(self, other):
        self.val = self.val // other
        return self.val
    def __ifloordiv__(self,other):
        self.val = self.val // other
        return self

class B:
    val = 2
    def __rfloordiv__(self,other):
        return self.val // other
    def __ifloordiv__(self,other):
        return self.val // other

foo = A()
bar = B()
check_int1(foo // 2)
check_int2(foo.val)
check_int3(3 // bar)
check_int4(bar.val)
check_int5(foo // bar)
check_int6(foo.val)
check_int7(bar.val)
foo //= 3
check_int8(foo.val)
bar //= 1000
check_int9(bar)
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        // Checking for equility with floats is okay here because the are all unambiguous
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)5);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)0);
        state.eval();
    }
    
    SECTION( "of the ** operator"){
        auto code = build_string(R"(
class A:
    val = 3.5
    def __pow__(self, other):
        self.val = self.val ** other
        return self.val
    def __ipow__(self,other):
        self.val = self.val ** other
        return self

class B:
    val = 2
    def __rpow__(self,other):
        return self.val ** other
    def __ipow__(self,other):
        return self.val ** other

foo = A()
bar = B()
check_int1(foo ** 2)
check_int2(foo.val)
check_int3(3 ** bar)
check_int4(bar.val)
foo ** bar
check_int7(bar.val)
foo **= 3
bar **= 5
check_int9(bar)
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((double)12.25);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((double)12.25);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)8);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        //(*(state.ns_builtins))["check_int5"] = make_builtin_check_value((double)4870.99);
        //(*(state.ns_builtins))["check_int6"] = make_builtin_check_value((double)4870.99);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        //(*(state.ns_builtins))["check_int8"] = make_builtin_check_value((double)1.15572e+11);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)32);
        state.eval();
    }

    SECTION( "of the << operator"){
        auto code = build_string(R"(
class A:
    val = 3
    def __lshift__(self, other):
        self.val = self.val << other
        return self.val
    def __ilshift__(self,other):
        self.val = self.val << other
        return self

class B:
    val = 2
    def __rlshift__(self,other):
        return self.val << other
    def __ilshift__(self,other):
        return self.val << other

foo = A()
bar = B()
check_int1(foo << 2)
check_int2(foo.val)
check_int3(3 << bar)
check_int4(bar.val)
check_int5(foo << bar)
check_int6(foo.val)
check_int7(bar.val)
foo <<= 1
check_int8(foo.val)
bar <<= 3
check_int9(bar)
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)12);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)12);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)16);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)8192);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((int64_t)8192);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((int64_t)16384);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)16);
        state.eval();
    }

    SECTION( "of the >> operator"){
        auto code = build_string(R"(
class A:
    val = 3
    def __rshift__(self, other):
        self.val = self.val >> other
        return self.val
    def __irshift__(self,other):
        self.val = self.val >> other
        return self

class B:
    val = 2
    def __rrshift__(self,other):
        return self.val >> other
    def __irshift__(self,other):
        return self.val >> other

foo = A()
bar = B()
check_int1(foo >> 2)
check_int2(foo.val)
check_int3(3 >> bar)
check_int4(bar.val)
check_int5(foo >> bar)
check_int6(foo.val)
check_int7(bar.val)
foo >>= 1
check_int8(foo.val)
bar >>= 3
check_int9(bar)
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((int64_t)1);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)0);
        state.eval();
    }

    SECTION( "of the & operator"){
        auto code = build_string(R"(
class A:
    val = 3
    def __and__(self, other):
        self.val = self.val & other
        return self.val
    def __iand__(self,other):
        self.val = self.val & other
        return self

class B:
    val = 2
    def __rand__(self,other):
        return self.val & other
    def __iand__(self,other):
        return self.val & other

foo = A()
bar = B()
check_int1(foo & 2)
check_int2(foo.val)
check_int3(3 & bar)
check_int4(bar.val)
check_int5(foo & bar)
check_int6(foo.val)
check_int7(bar.val)
foo &= 1
check_int8(foo.val)
bar &= 3
check_int9(bar)
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)2);
        state.eval();
    }

    SECTION( "of the | operator"){
        auto code = build_string(R"(
class A:
    val = 3
    def __or__(self, other):
        self.val = self.val | other
        return self.val
    def __ior__(self,other):
        self.val = self.val | other
        return self

class B:
    val = 2
    def __ror__(self,other):
        return self.val | other
    def __ior__(self,other):
        return self.val | other

foo = A()
bar = B()
check_int1(foo | 2)
check_int2(foo.val)
check_int3(3 | bar)
check_int4(bar.val)
check_int5(foo | bar)
check_int6(foo.val)
check_int7(bar.val)
foo |= 1
check_int8(foo.val)
bar |= 3
check_int9(bar)
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)3);
        state.eval();
    }

    SECTION( "of the ^ operator"){
        auto code = build_string(R"(
class A:
    val = 3
    def __xor__(self, other):
        self.val = self.val ^ other
        return self.val
    def __ixor__(self,other):
        self.val = self.val ^ other
        return self

class B:
    val = 2
    def __rxor__(self,other):
        return self.val ^ other
    def __ixor__(self,other):
        return self.val ^ other

foo = A()
bar = B()
check_int1(foo ^ 2)
check_int2(foo.val)
check_int3(3 ^ bar)
check_int4(bar.val)
check_int5(foo ^ bar)
check_int6(foo.val)
check_int7(bar.val)
foo ^= 1
check_int8(foo.val)
bar ^= 3
check_int9(bar)
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)1);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)1);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)1);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((int64_t)3);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)1);
        state.eval();
    }

    SECTION( "of the % operator"){
        auto code = build_string(R"(
class A:
    val = 3
    def __mod__(self, other):
        self.val = self.val % other
        return self.val
    def __imod__(self,other):
        self.val = self.val % other
        return self

class B:
    val = 2
    def __rmod__(self,other):
        return self.val % other
    def __imod__(self,other):
        return self.val % other

foo = A()
bar = B()
check_int1(foo % 2)
check_int2(foo.val)
check_int3(3 % bar)
check_int4(bar.val)
check_int5(foo % bar)
check_int6(foo.val)
check_int7(bar.val)
foo %= 1
check_int8(foo.val)
bar %= 3
check_int9(bar)
    )");
        InterpreterState state(code);
        builtins::inject_builtins(state.ns_builtins);
        (*(state.ns_builtins))["check_int1"] = make_builtin_check_value((int64_t)1);
        (*(state.ns_builtins))["check_int2"] = make_builtin_check_value((int64_t)1);
        (*(state.ns_builtins))["check_int3"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int4"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int5"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int6"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int7"] = make_builtin_check_value((int64_t)2);
        (*(state.ns_builtins))["check_int8"] = make_builtin_check_value((int64_t)0);
        (*(state.ns_builtins))["check_int9"] = make_builtin_check_value((int64_t)2);
        state.eval();
    }

}
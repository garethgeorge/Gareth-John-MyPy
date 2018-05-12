def check_int1(v):
    print(v)
def check_int2(v):
    print(v)
def check_int3(v):
    print(v)
def check_int4(v):
    print(v)
def check_int5(v):
    print(v)
def check_int6(v):
    print(v)
def check_int7(v):
    print(v)
def check_int8(v):
    print(v)
def check_int9(v):
    print(v)

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

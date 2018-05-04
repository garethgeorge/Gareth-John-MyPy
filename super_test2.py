class A:
    def foo(self):
        print("A")
        return 3

class B(A):
    def foo(self):
        print("B")
        super().foo()
        super().foo()
        super().foo()
        super().foo()
        print("B Done")
        return 2

class C(A):
    def foo(self):
        print("C")
        super().foo()
        super().foo()
        super().foo()
        super().foo()
        super().foo()
        print("C Done")
        return 1

class D(B,C):
    def foo(self):
        print("D")
        return super().foo()

d = D()
print(d.foo())

class A:
    def func(self):
        print("F")

class C(A):
    def func(self):
        print("F!")

class B(A):
    pass

class D(C):
    pass

class E(B,D):
    pass

e = E()
e.func()

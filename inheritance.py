class A:
    val = 2
    def func(self):
        print(self.val)
    def __init__(self):
        print("A init called")
        self.val = 3

class B(A):
    def __init__(self):
        print("B init called")
        self.func()
        self.val = 5
        self.func()
        print("B init ended")


class C(A):
    arg = 100

class D(A):
    def func(self):
        print(str(self.val) + "!")
    def __init__(self):
        print("D init called")
        self.val = 10

class E(B,C,D):
    def lol(self):
        print(self.arg)

print("")
print("A:")
a = A()
a.func()
print("")
print("B:")
b = B()
b.func()
print("")
print("C:")
c = C()
c.func()
print("")
print("D:")
d = D()
d.func()
print("")
print("E:")
e = E()
e.func()
e.lol()

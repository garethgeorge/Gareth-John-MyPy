class A:
    h = 5
    def func(self):
        print(self.h)

class B(A):
    d = 7

class C(A):
    c = 8
    #def __init__(self):
    #    self.func()
    #    self.h = 10
    #    self.func()

class D(A,B,C):
    g = 9
    #def func(self):
    #    print(self.g)

e = D()
print(e.c)
f = D()
f.func()
g = D()
g.func()
h = D()
h.func()
h = D()
i = D()
i.func()
j = D()
k = D()
l = D()

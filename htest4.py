class A:
    val = 5

class B(A):
    pass

class C(B):
    pass

c = C()
print(c.val)
B.val = 10
print(c.val)

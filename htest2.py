class A:
    thing = 4

class B(A):
    thing2 = 7

c = B()
print(c.thing)
A.thing = 9
print(c.thing)

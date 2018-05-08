class add_overload:
    val = 5
    def __add__(self,other):
        self.val = self.val + other
        return(self.val)
    def __radd__(self,other):
        self.val = other + self.val
        return(self.val)
    def __init__(self,myval):
        self.val = myval

a1 = add_overload(10)
a2 = add_overload(20)

print("1")
print(a1 + 1)
print(a1.val)

print("2")
print(2 + a1)
print(a1.val)

print("3")
print(a1 + a2)
print(a1.val)
print(a2.val)

a1.val = 1
a2.val = 5
print("4")
print(a2 + a1)
print(a1.val)
print(a2.val)

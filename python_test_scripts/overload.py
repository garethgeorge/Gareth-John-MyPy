class add_overload:
    val = 5
    def __add__(self,other):
        print("in add")
        self.val = self.val + other
        return(self.val)
    def __radd__(self,other):
        print("in radd")
        self.val = other - self.val
        return(self.val)
    def __init__(self,myval):
        self.val = myval

class wow:
    cool = 9
    def __radd__(self,other):
        print("in cool");
        self.cool = other + self.cool
        return(self.cool)

a1 = add_overload(10)
a2 = add_overload(20)

print("1")
print(a1 + 1.1)
print(a1.val)

print("2")
print(2 + a1)
print(a1.val)

print("3")
print(a1 + a2)
print(a1.val)
print(a2.val)


print("4")
c1 = wow()
print(a2 + c1)
print(a1.val)
print(c1.cool)

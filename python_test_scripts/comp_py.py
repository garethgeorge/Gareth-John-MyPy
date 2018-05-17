class foo:
    val = 7
    def __lt__(self,other):
        return True
    def __gt__(self,other):
        return 5
    def __eq__(self,other):
        self.val = self.val - 1
        return self.val == 5
    def __ne__(self,other):
        return other != self.val
    def __le__(self,other):
        return other
    def __ge__(self,other):
        return self.val

a = foo()
print(a < 5)
print(a > 6)
print(5 > a)
print(6 < a)
print(a == "s")
print(a == True)
print(a != 4)
print(a != 5)
print(a <= 9)
print(a >= 6)
print(9 <= a)
print(6 >= a)

class testclass:
    val = 6
    def __sub__ (self,other):
        return self.val - other
    def __isub__(self, other):
        self.val = self.val + other
        return self

a = testclass();
print(a - 3)
print(a.val)
a -= 10
print(a.val)

b = 10
b -= 3
print(b)

class tmp:
    def __add__(self,other):
        return 5 + other

def ctest(a,b):
    def so():
        return (a + b)
    return so

d = tmp()
c = ctest(d,5)
print(c())

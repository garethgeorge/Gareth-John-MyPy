def func(a,b,c):
    print(b)
    def foo():
        return (a + c);
    return foo

d = func(1,2,3)
print(d())

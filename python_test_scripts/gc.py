def func(a, b):
    return a + b

x = 0
while x < 1000000:
    x = func(x, 1)
print(x)
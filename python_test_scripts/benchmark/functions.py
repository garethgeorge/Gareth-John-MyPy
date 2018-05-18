def func(a, b):
    return a + b 

x = 0
y = 0
while x < 10000000:
    y += func(x, x)
print(y)
def func(a, b):
    return a + b

x = 0
while x < 10000000:
    func(1, 1)
    x += 1

print(x)
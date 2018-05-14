def range(n):
    x = 0
    while x < n:
        yield x 
        x += 1

for x in range(10):
    print(x)
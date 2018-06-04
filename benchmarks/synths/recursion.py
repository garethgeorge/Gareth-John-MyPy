def recurse(depth):
    if depth <= 0:
        return 1
    else:
        return recurse(depth - 1)
    
x = 1
while x < 200000:
    x += recurse(x % 100)

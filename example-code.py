def range222():
    x = 1
    while x < 10:
        yield x
        x += 1

generator = range222()
# print("this is a generator:", generator)

# for x in generator:
#     print(x)
acum = 0
def gen(gen):
    for x in gen:
        yield x * 2

for x in gen(x - 1 for x in (1+x for x in range(0, 1000000))):
    acum += x 
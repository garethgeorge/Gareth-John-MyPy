mylist = [1] * 10000000

def enumerate(l):
    i = 0
    for v in l:
        yield i, v
        i += 1

count = 0
for k, v in enumerate(l):
    count += k 
print(count)
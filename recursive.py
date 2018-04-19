def shownums(a):
    print(a)
    if(a == 0):
        return
    else:
        shownums(a - 1)

shownums(5)

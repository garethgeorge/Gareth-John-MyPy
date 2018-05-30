list = [1,2,3,4,5]
print(list)
print(list[1:2])

def enumerate(a):
  i = 0
  for x in a:
    yield i, x 
    i = i + 1

# for x, y in enumerate([1, 2, 3, 4, 5, 6]):
#  print(x)
#   print(y)

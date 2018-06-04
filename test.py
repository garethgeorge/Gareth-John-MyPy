def enumerate(list):
  x = 0
  for val in list:
    yield x, val 
    x += 1

list = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
for x, val in enumerate(list):
  print(x, val)
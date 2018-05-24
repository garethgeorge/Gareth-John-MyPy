a = "test"
b = "foobar"
collect_garbage()

c = [a, b]
d = ["biz", 5, "baz"]

collect_garbage()
collect_garbage()
collect_garbage()

def make_list():
  foob = [1, 5, 6]
  foob.append("test")
  collect_garbage()
  return foob 
val = make_list()

print(val)
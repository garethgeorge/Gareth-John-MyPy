a = "test"
b = "foobar"
c = [a, b]
d = ["biz", 5, "baz"]

def myGenerator():
    x = 0
    while x < 10:
        yield "this is a string value from a generator"

collect_garbage()
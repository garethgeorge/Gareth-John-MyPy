class simple_class:
    val = 4
    buf = 6
    def show(selfa,y):
        print(selfa.val + y)
        return 1
    def __init__(self,r=6):
        print("Initing!")
        self.val = r

bar = simple_class(2)
boo = simple_class(9)
bar.show(boo.show(5))
print(bar.val)
print(simple_class.val)
print(bar.buf)
print(simple_class.buf)
print("Whoa!")
simple_class.val = 100
simple_class.buf = 101
bar.show(10)
print(bar.val);
print(simple_class.val)
print(bar.buf)
print(simple_class.buf)
print("Whoa 2!");
bar.buf = -100;
print(bar.val);
print(simple_class.val)
print(bar.buf)
print(simple_class.buf)
print("Whoa 3!")
bar.uhoh = 5
print(bar.uhoh)
simple_class.wow = 10
print(simple_class.wow)
print(bar.wow)

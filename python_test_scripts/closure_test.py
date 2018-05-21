def print_msg(msg):
    def printer():
        return msg
    return printer

def ret_msg(msg):
    def printer(alt):
        def b():
            return(msg + 4)
        return b
    return printer(msg)

def print_msg_store(msg):
    msg = -1
    def printer():
        return msg
    return printer

weird = 10

class Ah():
    val = weird
    def getfunc(self,something):
        def bar():
            return(self.val + something)
        return bar

a = print_msg(100)
print(a())
b = print_msg(200)
print(b())
c = ret_msg(1)
print(c())
d = ret_msg(5.5)
print(d())
print(a())
e = print_msg_store(300)
print(e())
f = Ah()
g = f.getfunc(10)
print(g())
f.val = 12
print(g())
h = Ah()
i = h.getfunc(30)
print(i())
weird = 90
print(i())

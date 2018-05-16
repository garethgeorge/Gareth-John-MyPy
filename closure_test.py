def print_msg(msg):
    def printer():
        print(msg)
    return printer

def ret_msg(msg):
    def printer(alt):
        def b():
            return(msg + 4)
        return b
    return printer(msg)

def print_msg_store(msg):
    msg = "hmm"
    def printer():
        print(msg)
    return printer

weird = 10

class Ah:
    val = weird
    def getfunc(self,something):
        def b():
            return(self.val + something)
        return b



a = print_msg("fun")
a()
b = print_msg("ooo")
b()
c = ret_msg(1)
print(c())
d = ret_msg(5.5)
print(d())
a()
e = print_msg_store("oh!")
e()
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

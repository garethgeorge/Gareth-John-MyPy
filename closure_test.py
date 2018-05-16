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

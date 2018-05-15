def print_msg(msg):
    def printer():
        print(msg)
    return printer

def ret_msg(msg):
    def printer():
        return(msg + 4)
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

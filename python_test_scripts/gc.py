def print_msg(msg):
    def printer():
        return msg
    return printer

a = print_msg(100)
print(a())
b = print_msg(200)
print(b())
print(a())
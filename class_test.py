#class simplest_class:
#    a = 2

class simple_class:
    val = 4
    def show(self,y):
        print(self.val + y)    
    def __init__(self,r=6):
        self.val = r



#foo = simplest_class()
#foo.a = 5
#print(foo.a)
bar = simple_class(2)
bar.show(10)
print(simple_class.val)

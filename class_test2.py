class A:
    def foo(self,x):
        print ("executing foo " + str(self) + " " + str(x))    
    @classmethod
    def class_foo(cls,x):
        print ("executing class_foo " + str(cls) + " " + str(x))
    @staticmethod
    def static_foo(x):
        print ("executing static_foo " + str(x))

a = A()
a.foo(1)
a.class_foo(2)
a.static_foo(3)

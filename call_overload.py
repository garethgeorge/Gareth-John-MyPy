class testc:
    val = 0
    def __init__(self,v):
        self.val = v
    def __call__(self,a,b):
        return(a+b+self.val)

a = testc(100)
print(a(1,2))

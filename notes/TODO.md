# TODO

 - correctly read constant pool (add more type information to aid in decoding)
 - create all the different value types we need.
    - investigate using boost::variant for everything maybe
    - list of types
        - string
        - literal (numbers)
        - code
        - object
 - implement critical builtins 
    - print 
    - range
 - get arithmetic working, by this time the following program should work
```
print(3)
```
 - next thing would be nice
```
a = 7
print(a)
```
 - etc.

 - correctly read constant pool (add more type information to aid in decoding)
 - create all the different value types we need.
    - investigate using boost::variant for everything maybe
    - list of types
        - string
        - literal (numbers)
        - code
        - object

# TODO
 - implement functions - John/Jack
 - implement MAKE_CLOSURE - John/Jack
 - implement Objects/Classes - John/Jack
 - implement extended arguments - John/Jack or Gareth
 - finish implement continue/break/try/except - John/Jack
 - implement garbage collection - Gareth
 - implement generators/iterators - Gareth
 - primitive types for Lists/Tupples/Map/Set - Gareth
 - kwarg functions - John/Jack
 - implement a ton of builtins for the rest of the quarter
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
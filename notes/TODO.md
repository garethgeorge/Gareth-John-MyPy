
 - correctly read constant pool (add more type information to aid in decoding)
 - create all the different value types we need.
    - investigate using boost::variant for everything maybe
    - list of types
        - string
        - literal (numbers)
        - code
        - object

# TODO
 - implement MAKE_CLOSURE - John/Jack
 - implement Objects/Classes - John/Jack
 - implement variadic functions - John Jack
 - finish implement continue/break/try/except - John/Jack
 - kwarg functions - John/Jack
 - find a testing framework that specifically tests python interpreters - John/Jack or Garth
 - implement garbage collection - Gareth
 - implement generators/iterators - Gareth
 - primitive types for Lists/Tupples/Map/Set - Gareth
 - implement a ton of builtins for the rest of the quarter
    - print 
    - range
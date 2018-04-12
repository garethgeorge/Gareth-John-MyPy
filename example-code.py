def main_py():
  # this is interesting, if we look at the disassembly for this function statement we get
  """
  1           0 LOAD_CONST               0 (<code object main_py at 0x10322fae0, file "sys.stdin.py", line 1>)
              2 LOAD_CONST               1 ('main_py')
              4 MAKE_FUNCTION            0
              6 STORE_NAME               0 (main_py)
  """
  # we can see taht the code object is actually stored in the constant table by python
  # it then gets loaded in and compiled at runtime :o
  # very interesting.
  print("hello world")
  print("foo bar baz")
  print("whaz up chicken")

a = 1
b = 2
c = 1 + 2
print(c)
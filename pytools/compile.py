import sys, dis, json, base64, argparse

parser = argparse.ArgumentParser(
  description="our simple pyc compiler leveraging the 'compile' method provided " + 
  "to us by python"
)

parser.add_argument(
  "--file", 
  default="sys.stdin",
  help="the file to read the input from",
)

parser.add_argument(
  "--dis",
  action="store_true",
  help="should we print the dis rather than the json form",
  required=False
)

args = parser.parse_args()

if args.file == "sys.stdin":
  code = compile(sys.stdin.read(), "sys.stdin.py", "exec")
else:
  with open(args.file, "r") as f:
    code = compile(f.read(), args.file, "exec")

if args.dis:
  dis.dis(code)
else:
  # to deeply understand this object
  # https://late.am/post/2012/03/26/exploring-python-code-objects.html
  json.dump({
    "bytecode": base64.encodebytes(code.co_code).decode('ascii'),
    "lnotab": base64.encodebytes(code.co_lnotab).decode('ascii'), # the line number table
  }, sys.stdout, indent=2)
  # print(dir(code))
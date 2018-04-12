import argparse 
import dis, marshal, sys


parser = argparse.ArgumentParser(
  description="a program that will take the bytecode file and convert it to " + 
  "an easily parsable JSON format.",
)

parser.add_argument("pycfile", help="the pyc file that we are to load in!")

args = parser.parse_args()

header_size = 12 if sys.version_info >= (3, 3) else 8

with open(args.pycfile, "rb") as f:
  f.read(header_size)
  code = marshal.load(f)                     # rest is a marshalled code object

print(code)
# print(dis.dis(code))
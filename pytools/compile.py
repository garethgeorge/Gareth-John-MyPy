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

parser.add_argument(
    "--indentJSON",
    action="store_true",
    help="should we indent JSON output",
)

args = parser.parse_args()

if args.file == "sys.stdin":
    code_base = compile(sys.stdin.read(), "sys.stdin.py", "exec")
else:
    with open(args.file, "r") as f:
        code_base = compile(f.read(), args.file, "exec")

if args.dis:
    dis.dis(code_base)
else:
    # so obviously this takes a tiny program and blows it up into a HUGE json file
    # which is not great, but it is VERY easy for us to parse into the c interpreter
    # if we choose to go that way, or just save out as an intermediate representation
    # perhaps in some compressed form for large enough programs
    code_type = type(code_base)
    def jsonify_code(code):
        def helper(obj):
            if type(obj) == code_type:
                return jsonify_code(obj)
            else:
                return {
                    "type": "literal",
                    "real_type": str(type(obj)),
                    "value": obj
                }
        
        # to deeply understand this object
        # https://late.am/post/2012/03/26/exploring-python-code-objects.html
        return { # TODO: consider converting to some binary format instead of JSON
            "type": "code",
            "co_code": base64.encodebytes(code.co_code).decode('ascii').replace("\n", ""), # our C++ base64 decoder does not take kindly to new lines.
            "co_lnotab": base64.encodebytes(code.co_lnotab).decode('ascii').replace("\n", ""), # the line number table
            "co_consts": list(map(helper, code.co_consts)),
            "co_name": code.co_name,
            "co_filename": code.co_filename,
            "co_argcount": code.co_argcount,
            "co_kwonlyargcount": code.co_kwonlyargcount,
            "co_nlocals": code.co_nlocals,
            "co_stacksize": code.co_stacksize,
            "co_names": code.co_names or None,
            "co_varnames": code.co_varnames or None,
            "co_freevars": code.co_freevars or None,
            "co_cellvars": code.co_cellvars or None, 
        }
    if args.indentJSON:
        json.dump(jsonify_code(code_base), sys.stdout, indent=2, sort_keys=True)
    else:
        json.dump(jsonify_code(code_base), sys.stdout)
    print("")
import dis, json, sys

with open("oplist.hpp", "w") as hpp, open("oplist.cpp", "w") as cpp:
    hpp.writelines([
      "#ifndef OPLIST_H\n",
      "#define OPLIST_H\n",
      "#include <stdint.h>\n",
    ])
    cpp.writelines([
      "#include \"oplist.hpp\"\n"
    ])

    hpp.write("namespace py {\nnamespace op {\n")
    hpp.write("extern const char *name[%d];\n" % len(dis.opname))

    # begin write argcount function
    hpp.write(
"""
const uint8_t HAVE_ARGUMENT = 0x%x;
inline uint8_t operand_length(uint8_t opcode) {
    return opcode >= HAVE_ARGUMENT ? 2 : 0;
}

""" % (dis.HAVE_ARGUMENT, dis.EXTENDED_ARG))

    cpp.write("namespace py {\nnamespace op {\n")
    
    # begin write op list
    cpp.write("const char *name[] = {\n\t")
    cpp.write(",\n    ".join("\"%s\"" % opname for opname in dis.opname))
    cpp.write("\n};\n")
    # end write op list
    cpp.write("}\n}\n")

    # hpp write list of arg classes
    for opcode, opname in enumerate(dis.opname):
        if opname[0] == '<': continue 
        hpp.write("const uint8_t %-15s = 0x%02x;\n" % (opname, opcode))

    hpp.writelines([
        "}\n"
        "}\n"
        "#endif\n"
    ])


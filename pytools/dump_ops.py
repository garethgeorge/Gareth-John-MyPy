import dis, json, sys
sys.stdout.write("const char *oplist[] = ")
sys.stdout.write("{\n\t\"%s\"\n}" % "\",\n\t\"".join(dis.opname))
sys.stdout.write(";\n")
print("\n".join(["const uint8_t %s = %d;" % (op, idx) for idx, op in enumerate(dis.opname) if op[0] != '<']))

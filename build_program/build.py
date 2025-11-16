# TODO: Rewrite this in Rhino!

data = []

def is_arg(s):
    return len(s) > 2 and s[1] == ":"

with open("docs/opcodes.txt", "r") as f:
    for line in f:
        if line[0] == "#": continue

        segment = line.split()
        
        if len(segment) == 0: continue

        op = segment[0]
        args = []
        payload = None
        i = 1

        while is_arg(segment[i]):
            arg = segment[i].split(":")
            args.append(arg)
            if arg[0] == "P":
                payload = arg[1]
            i += 1

        desc = (" ").join(segment[i:])

        data.append({
            "op": op,
            "enum": "OP_" + op,
            "args": args,
            "payload": payload,
            "desc": desc,
        })

def create_include(path):
    f =  open("compiler/include/" + path, "w")
    f.write("// This file was generated automatically by build_program/build.py\n\n")
    return f


def as_c_data_type(v):
    if v == "r": return "vm_reg"
    if v == "u": return "uint8_t"
    if v == "pc": return "uint16_t"
    return v

with create_include("op_code_list.c") as f:
    f.write("#define OP_CODE(MACRO) \\\n")
    for ins in data:
        f.write("\tMACRO(" + ins["enum"] + ") \\\n")
    f.write("\n\n")

with create_include("emit_op_code.c") as f:
    for ins in data:
        f.write("// " + ins["desc"] + "\n")
        f.write("size_t emit_" + ins["op"].lower() + "(Unit* unit")

        for arg in ins["args"]:
            f.write(", " + as_c_data_type(arg[1]) + " " + arg[0])
        
        f.write(")\n{\n")

        f.write("\tsize_t i = unit->count++;\n")
        f.write("\tunit->instruction[i].op = " + ins["enum"] + ";\n")

        for arg in ins["args"]:
            if arg[0] == "P": continue
            f.write("\tunit->instruction[i]." + arg[0].lower() + " = " + arg[0].lower() + ";\n")
    
        if ins["payload"]:
            f.write("\t\n")
            f.write("\tunion\n")
            f.write("\t{\n")
            f.write("\t\t" + ins["payload"] + " data;\n")
            f.write("\t\tuint32_t word[wordsizeof(" + ins["payload"] + ")];\n")
            f.write("\t} as = {.data = P};\n")
            f.write("\tfor (size_t i = 0; i < wordsizeof(" + ins["payload"] + "); i++)\n")
            f.write("\t\tunit->instruction[unit->count++].word = as.word[i];\n")
            f.write("\t\n")
    
        f.write("\treturn i;\n")

        f.write("}\n\n")
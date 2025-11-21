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
    if v == "i": return "uint8_t"
    return v

def as_c_format_string(T):
    if T == "double": return "%f"
    return "0x%p"

with create_include("op_code_list.c") as f:
    f.write("#define OP_CODE(MACRO) \\\n")
    for ins in data:
        f.write("\tMACRO(" + ins["enum"] + ") \\\n")
    f.write("\n\n")

with create_include("emit_op_code.c") as f:
    for ins in data:
        f.write("// " + ins["op"] + "\n")
        f.write("// " + ins["desc"] + "\n")
        f.write("size_t emit_" + ins["op"].lower() + "(Unit* unit")

        for arg in ins["args"]:
            f.write(", " + as_c_data_type(arg[1]) + " " + arg[0].lower())
        
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
            f.write("\t} as = {.data = p};\n")
            f.write("\tfor (size_t i = 0; i < wordsizeof(" + ins["payload"] + "); i++)\n")
            f.write("\t\tunit->instruction[unit->count++].word = as.word[i];\n")
            f.write("\t\n")
    
        f.write("\treturn i;\n")

        f.write("}\n\n")

with create_include("printf_instruction.c") as f:
    f.write("size_t printf_instruction(Unit* unit, size_t i)\n{\n")
    f.write("\tInstruction ins = unit->instruction[i];\n")        
    f.write("\tprintf(\"\\x1b[90m%02X %02X %02X %02X  \", ins.b, ins.a, ins.x, ins.op);\n")
    f.write("\tprintf(\"\\x1b[37m%04X  \", i);\n")
    f.write("\tprintf(\"\\x1b[36m%-*s\", 13, op_code_string((OpCode)ins.op));\n")
    f.write("\ti++;\n\n")

    f.write("\tswitch (ins.op) {\n")
    
    for ins in data:
        f.write("\tcase " + ins["enum"] + ":")
        if ins["payload"]:
            f.write("\n\t{\n\t\t")
        else:
            f.write(" ")

        f.write("printf(\"")
        x_gap_filled = False
        for arg in ins["args"]:
            if arg[0] == "P": continue
            
            if arg[0] == "X":
                x_gap_filled = True
            elif not x_gap_filled: 
                x_gap_filled = True
                f.write("      ")

            f.write("  \\x1b[90m" + arg[0].lower() + " \\x1b[0m")
            
            if arg[0] == "Y": f.write("%04X")
            else: f.write("%02d")
        f.write("\\n\"")
        for arg in ins["args"]:
            if arg[0] == "P": continue
            else: f.write(", ins." + arg[0].lower())        
        f.write(");")

        if ins["payload"]:
            f.write("\n\n")
            f.write("\t\tunion\n")
            f.write("\t\t{\n")
            f.write("\t\t\t" + ins["payload"] + " value;\n")
            f.write("\t\t\tuint32_t word[wordsizeof(" + ins["payload"] + ")];\n")
            f.write("\t\t\tuint8_t byte[wordsizeof(" + ins["payload"] + ")][4];\n")
            f.write("\t\t} as;\n")
            f.write("\t\tfor (size_t j = 0; j < wordsizeof(" + ins["payload"] + "); j++)\n")
            f.write("\t\t\tas.word[j] = unit->instruction[i++].word;\n\n")

            f.write("\t\tprintf(\"\\x1b[90m%02X %02X %02X %02X        " + as_c_format_string(arg[1]) + "\\n\", as.byte[0][3], as.byte[0][2], as.byte[0][1], as.byte[0][0], as.value);\n")
            f.write("\t\tfor (size_t j = 1; j < wordsizeof(" + ins["payload"] + "); j++)\n")
            f.write("\t\t\tprintf(\"%02X %02X %02X %02X\\n\", as.byte[j][3], as.byte[j][2], as.byte[j][1], as.byte[j][0]);\n")

            f.write("\t\tbreak;\n")
            f.write("\t}\n")
        else:
            f.write(" break;\n")

    f.write("\t}\n\n")
    f.write("\tprintf(\"\\x1b[0m\");\n")
    f.write("\treturn i;\n")

    f.write("}\n\n")

with create_include("get_payload_size.c") as f:
    f.write("size_t get_playload_size(OpCode op)\n")
    f.write("{\n")
    for ins in data:
        if ins["payload"]:
            f.write("\tif (op == " + ins["enum"] + ")\n")
            f.write("\t\treturn wordsizeof(" + ins["payload"] + ");\n")
    f.write("\n\treturn 0;\n")
    f.write("}\n")

payloadTypes = []
for ins in data:
    if ins["payload"] and not ins["payload"] in payloadTypes:
        payloadTypes.append(ins["payload"])

def strip_typename(name):
    return name.lower().replace(" ", "").replace("*", "_ptr")

with create_include("patch_payload.c") as f:
    for payload in payloadTypes:
        f.write("void patch_" + strip_typename(payload) + "_payload(Unit* unit, size_t location, "+ payload+" p)\n" )
        f.write("{\n")
        f.write("\tunion\n")
        f.write("\t{\n")
        f.write("\t\t" + payload + " data;\n")
        f.write("\t\tuint32_t word[wordsizeof(" + payload + ")];\n")
        f.write("\t} as = {.data = p};\n")
        f.write("\tfor (size_t i = 0; i < wordsizeof(" + payload + "); i++)\n")
        f.write("\t\tunit->instruction[location + i].word = as.word[i];\n")
        f.write("}\n\n")
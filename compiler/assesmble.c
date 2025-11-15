#include "assemble.h"

void assemble(Compiler *compiler, Program *apm, ByteCode *byte_code)
{
    byte_code->byte[0] = PUSH_INT_IMMEDIATE;
    byte_code->byte[1] = 42;
    byte_code->byte[2] = OUTPUT_INT_VALUE;
    byte_code->byte_count = 3;
}
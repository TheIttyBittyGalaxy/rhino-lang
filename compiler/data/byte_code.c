#include "byte_code.h"

DEFINE_ENUM(INSTRUCTION, Instruction, instruction)

void init_byte_code(ByteCode *byte_code)
{
    for (size_t i = 0; i < 128; i++)
        byte_code->byte[i] = 0xFF;

    byte_code->byte_count = 0;
}
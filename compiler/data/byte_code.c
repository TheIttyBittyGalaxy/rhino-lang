#include "byte_code.h"

DEFINE_ENUM(OP_CODE, OpCode, op_code)

void init_byte_code(ByteCode *byte_code)
{
    for (size_t i = 0; i < 128; i++)
        byte_code->instruction[i].word = 0x00000000;

    byte_code->count = 0;
}

size_t printf_instruction(ByteCode *byte_code, size_t i)
{
    Instruction ins = byte_code->instruction[i++];

    printf("\x1b[90m%04X\x1b[0m  \x1b[34m%-*s\x1b[0m  %02x %02x %02x",
           i,
           21, op_code_string((OpCode)ins.op),
           ins.a, ins.b, ins.c);

    int playload = 0;
    if (ins.op == LOAD_INT)
        playload = wordsizeof(int);
    else if (ins.op == LOAD_NUM)
        playload = wordsizeof(double);
    else if (ins.op == LOAD_STR)
        playload = wordsizeof(char *);
    else
    {
        printf("\n");
        return i;
    }

    printf("\x1b[90m");
    while (playload--)
    {
        Instruction data = byte_code->instruction[i++];
        printf("  %02x %02x %02x %02x", data.op, data.a, data.b, data.c);
    }
    printf("\x1b[0m\n");

    return i;
}
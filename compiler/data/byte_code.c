#include "byte_code.h"

DEFINE_ENUM(OP_CODE, OpCode, op_code)

void init_byte_code(ByteCode *byte_code)
{
    byte_code->init = NULL;
    byte_code->main = NULL;
}

void init_unit(Unit *unit)
{
    for (size_t i = 0; i < 128; i++)
        unit->instruction[i].word = 0x00000000;

    unit->count = 0;
}

size_t printf_instruction(Unit *unit, size_t i)
{
    Instruction ins = unit->instruction[i++];

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
    else if (ins.op == CALL)
        playload = wordsizeof(Unit *);
    else
    {
        printf("\n");
        return i;
    }

    printf("\x1b[90m");
    while (playload--)
    {
        Instruction data = unit->instruction[i++];
        printf("  %02x %02x %02x %02x", data.op, data.a, data.b, data.c);
    }
    printf("\x1b[0m\n");

    return i;
}

void printf_unit(Unit *unit)
{
    printf("UNIT %p\n", unit);
    size_t i = 0;
    while (i < unit->count)
        i = printf_instruction(unit, i);
    printf("\n");
}

void printf_byte_code(ByteCode *byte_code)
{
    printf_unit(byte_code->init);
    printf_unit(byte_code->main);
}
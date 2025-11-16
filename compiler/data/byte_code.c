#include "byte_code.h"

DEFINE_ENUM(OP_CODE, OpCode, op_code)

#include "../include/get_payload_size.c"

void init_byte_code(ByteCode *byte_code)
{
    byte_code->init = NULL;
    byte_code->main = NULL;
}

void init_unit(Unit *unit)
{
    unit->parameter_count = 0;
    unit->register_count = 0;

    for (size_t i = 0; i < 128; i++)
        unit->instruction[i].word = 0x00000000;
    unit->count = 0;

    unit->nested_in = NULL;
    unit->next = NULL;
}

size_t printf_instruction(Unit *unit, size_t i)
{
    Instruction ins = unit->instruction[i];

    printf("\x1b[90m%04X\x1b[0m  \x1b[34m%-*s\x1b[0m  %02x %02x %02x",
           i,
           21, op_code_string((OpCode)ins.op),
           ins.x, ins.a, ins.b);

    i++;

    size_t playload = get_playload_size((OpCode)ins.op);
    if (playload > 0)
    {
        printf("\x1b[90m");
        while (playload--)
        {
            Instruction data = unit->instruction[i++];
            printf("  %02x %02x %02x %02x", data.op, data.x, data.a, data.b);
        }
        printf("\x1b[0m");
    }

    printf("\n");
    return i;
}

void printf_unit(Unit *unit)
{
    size_t i = 0;
    while (i < unit->count)
        i = printf_instruction(unit, i);
    printf("\n");
}

void printf_byte_code(ByteCode *byte_code)
{
    Unit *unit = byte_code->init;
    while (unit)
    {
        printf("UNIT %p\n", unit);
        printf_unit(unit);
        unit = unit->next;
    }
}
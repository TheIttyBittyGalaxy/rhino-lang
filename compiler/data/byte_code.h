#ifndef BYTE_CODE_H
#define BYTE_CODE_H

#include "../core.h"

// UTILITY //

typedef uint8_t vm_reg;

// Returns the number of 32 bit words needed to represent a data type
#define wordsizeof(T) (sizeof(T) - 1) / 4 + 1

// OP CODE //

#include "../include/op_code_list.c"

DECLARE_ENUM(OP_CODE, OpCode, op_code)

// INSTRUCTION //

typedef struct
{
    union
    {
        struct
        {
            uint8_t op;
            uint8_t x;
            uint8_t a;
            uint8_t b;
        };
        struct
        {
            uint16_t _;
            uint16_t y;
        };
        uint32_t word;
    };
} Instruction;

// BYTE CODE //

typedef struct Unit Unit;

struct Unit
{
    size_t register_count;

    Instruction instruction[1024];
    size_t count;

    Unit *nested_in;
    Unit *next; // TODO: This is currently used so that we can access all the units. Factor this in some better way.
};

typedef struct
{
    Unit *init;
    Unit *main;
} ByteCode;

void init_byte_code(ByteCode *byte_code);
void init_unit(Unit *unit);

size_t printf_instruction(Unit *unit, size_t i);
void printf_unit(Unit *unit);
void printf_byte_code(ByteCode *byte_code);

#endif
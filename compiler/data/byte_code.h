#ifndef BYTE_CODE_H
#define BYTE_CODE_H

#include "../core.h"

// UTILITY //

typedef uint8_t vm_reg;

// Returns the number of 32 bit words needed to represent a data type
#define wordsizeof(T) (sizeof(T) - 1) / 4 + 1

// OP CODE //

#define OP_CODE(MACRO)           \
    MACRO(INVALID_OP_CODE)       \
                                 \
    MACRO(CALL)                  \
    MACRO(JUMP)                  \
    MACRO(JUMP_IF_FALSE)         \
                                 \
    MACRO(MOVE)                  \
    MACRO(LOAD_NONE)             \
    MACRO(LOAD_TRUE)             \
    MACRO(LOAD_FALSE)            \
    MACRO(LOAD_INT)              \
    MACRO(LOAD_NUM)              \
    MACRO(LOAD_STR)              \
                                 \
    MACRO(INCREMENT)             \
    MACRO(DECREMENT)             \
                                 \
    MACRO(OP_NEG)                \
    MACRO(OP_NOT)                \
                                 \
    MACRO(OP_MULTIPLY)           \
    MACRO(OP_DIVIDE)             \
    MACRO(OP_REMAINDER)          \
    MACRO(OP_ADD)                \
    MACRO(OP_SUBTRACT)           \
    MACRO(OP_LESS_THAN)          \
    MACRO(OP_GREATER_THAN)       \
    MACRO(OP_LESS_THAN_EQUAL)    \
    MACRO(OP_GREATER_THAN_EQUAL) \
    MACRO(OP_EQUAL)              \
    MACRO(OP_NOT_EQUAL)          \
    MACRO(OP_LOGICAL_AND)        \
    MACRO(OP_LOGICAL_OR)         \
                                 \
    MACRO(OUTPUT_VALUE)

DECLARE_ENUM(OP_CODE, OpCode, op_code)

// INSTRUCTION //

typedef struct
{
    union
    {
        struct
        {
            uint8_t op;
            uint8_t a;
            uint8_t b;
            uint8_t c;
        };
        struct
        {
            uint16_t _;
            uint16_t x;
        };
        uint32_t word;
    };
} Instruction;

// BYTE CODE //

typedef struct
{
    Instruction instruction[1024];
    size_t count;
} Unit;

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
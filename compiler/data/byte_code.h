#ifndef BYTECODE_H
#define BYTECODE_H

#include "../core.h"

// Byte code instruction
#define INSTRUCTION(MACRO)     \
    MACRO(INVALID_INSTRUCTION) \
                               \
    MACRO(PUSH_INT_IMMEDIATE)  \
    MACRO(OUTPUT_INT_VALUE)

DECLARE_ENUM(INSTRUCTION, Instruction, instruction)

typedef struct
{
    uint8_t byte[128];
    size_t byte_count;
} ByteCode;

void init_byte_code(ByteCode *byte_code);

#endif
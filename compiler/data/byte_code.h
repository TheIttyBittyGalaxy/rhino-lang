#ifndef BYTECODE_H
#define BYTECODE_H

#include "../core.h"

// Byte code instruction
#define INSTRUCTION(MACRO)              \
    MACRO(INVALID_INSTRUCTION)          \
                                        \
    MACRO(DISCARD_STACK_VALUE)          \
                                        \
    MACRO(PUSH_NONE)                    \
    MACRO(PUSH_TRUE)                    \
    MACRO(PUSH_FALSE)                   \
    MACRO(PUSH_INT)                     \
    MACRO(PUSH_NUM)                     \
    MACRO(PUSH_STR)                     \
                                        \
    MACRO(PUSH_REGISTER_VALUE)          \
    MACRO(SET_REGISTER_VALUE)           \
                                        \
    MACRO(PUSH_THEN_INCREMENT_REGISTER) \
    MACRO(PUSH_THEN_DECREMENT_REGISTER) \
                                        \
    MACRO(OP_NEG)                       \
    MACRO(OP_NOT)                       \
                                        \
    MACRO(OP_MULTIPLY)                  \
    MACRO(OP_DIVIDE)                    \
    MACRO(OP_REMAINDER)                 \
    MACRO(OP_ADD)                       \
    MACRO(OP_SUBTRACT)                  \
    MACRO(OP_LESS_THAN)                 \
    MACRO(OP_GREATER_THAN)              \
    MACRO(OP_LESS_THAN_EQUAL)           \
    MACRO(OP_GREATER_THAN_EQUAL)        \
    MACRO(OP_EQUAL)                     \
    MACRO(OP_NOT_EQUAL)                 \
    MACRO(OP_LOGICAL_AND)               \
    MACRO(OP_LOGICAL_OR)                \
                                        \
    MACRO(OUTPUT_VALUE)

DECLARE_ENUM(INSTRUCTION, Instruction, instruction)

typedef struct
{
    uint8_t byte[2048];
    size_t byte_count;
} ByteCode;

void init_byte_code(ByteCode *byte_code);

#endif
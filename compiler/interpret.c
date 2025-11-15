#include "interpret.h"

#include "fatal_error.h"

void interpret(ByteCode *byte_code)
{
    size_t i = 0;

    int stack[128];
    size_t v = 0;

#define NEXT_BYTE() byte_code->byte[i++]
#define PUSH_STACK(_value) stack[v++] = _value
#define POP_STACK() stack[--v]

    while (i < byte_code->byte_count)
    {
        Instruction ins = (Instruction)NEXT_BYTE();
        switch (ins)
        {

        case PUSH_INT_IMMEDIATE:
            PUSH_STACK(NEXT_BYTE());
            break;

        case OUTPUT_INT_VALUE:
            printf("%d\n", POP_STACK());
            break;

        default:
            fatal_error("Could not interpret %s instruction (%02X) i = %d.", instruction_string(ins), ins, i);
            break;
        }
    }
}
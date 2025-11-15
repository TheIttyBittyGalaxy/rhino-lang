#include "interpret.h"

#include "fatal_error.h"

void output_to(RunOnString *output, const char *format, ...)
{
    if (!output)
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        return;
    }

    // FIXME: This could overflow
    char buffer[1024];

    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    append_run_on_string_with_terminator(output, buffer);
}

void interpret(ByteCode *byte_code, RunOnString *output_string)
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
            output_to(output_string, "%d\n", POP_STACK());
            break;

        default:
            fatal_error("Could not interpret %s instruction (%02X) i = %d.", instruction_string(ins), ins, i);
            break;
        }
    }

#undef NEXT_BYTE
#undef PUSH_STACK
#undef POP_STACK
}
#include "interpret.h"

#include "fatal_error.h"

// INTERPRETER VALUES //

#define RHINO_VALUE_KIND(MACRO) \
    MACRO(INVALID_RHINO_KIND)   \
                                \
    MACRO(RHINO_NONE)           \
    MACRO(RHINO_BOOL)           \
    MACRO(RHINO_INT)            \
    MACRO(RHINO_NUM)            \
    MACRO(RHINO_STR)

DECLARE_ENUM(RHINO_VALUE_KIND, RhinoValueKind, rhino_value_kind)
DEFINE_ENUM(RHINO_VALUE_KIND, RhinoValueKind, rhino_value_kind)

typedef struct
{
    RhinoValueKind kind;
    union
    {
        bool as_bool;
        int as_int;
        double as_num;
        char *as_str;
    };
} RhinoValue;

// IO //

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

// INTERPRET //

void interpret(ByteCode *byte_code, RunOnString *output_string)
{
    size_t program_counter = 0;

    RhinoValue stack[128];
    size_t v = 0;

#define NEXT_BYTE() byte_code->byte[program_counter++]

#define DECODE_BYTES(data, T)              \
    union                                  \
    {                                      \
        T value;                           \
        uint8_t bytes[sizeof(T)];          \
    } data;                                \
                                           \
    for (size_t i = 0; i < sizeof(T); i++) \
        data.bytes[i] = NEXT_BYTE();

#define PUSH_STACK(_value) stack[v++] = _value
#define POP_STACK() stack[--v]

    while (program_counter < byte_code->byte_count)
    {
        Instruction ins = (Instruction)NEXT_BYTE();

        switch (ins)
        {

        case PUSH_NONE:
        {
            RhinoValue value = {.kind = RHINO_NONE, .as_bool = false};
            PUSH_STACK(value);
            break;
        }

        case PUSH_TRUE:
        {
            RhinoValue value = {.kind = RHINO_BOOL, .as_bool = true};
            PUSH_STACK(value);
            break;
        }

        case PUSH_FALSE:
        {
            RhinoValue value = {.kind = RHINO_BOOL, .as_bool = false};
            PUSH_STACK(value);
            break;
        }

        case PUSH_INT:
        {
            DECODE_BYTES(data, int);
            RhinoValue value = {.kind = RHINO_INT, .as_int = data.value};
            PUSH_STACK(value);
            break;
        }

        case PUSH_NUM:
        {
            DECODE_BYTES(data, double);
            RhinoValue value = {.kind = RHINO_NUM, .as_num = data.value};
            PUSH_STACK(value);
            break;
        }

        case PUSH_STR:
        {
            DECODE_BYTES(data, char *);
            RhinoValue value = {.kind = RHINO_STR, .as_str = data.value};
            PUSH_STACK(value);
            break;
        }

        case OUTPUT_VALUE:
        {
            RhinoValue value = POP_STACK();
            if (value.kind == RHINO_NONE)
                output_to(output_string, "none\n");
            else if (value.kind == RHINO_BOOL)
                output_to(output_string, "%s\n", value.as_bool ? "true" : "false");
            else if (value.kind == RHINO_INT)
                output_to(output_string, "%d\n", value.as_int);
            else if (value.kind == RHINO_NUM)
                output_to(output_string, "%f\n", value.as_num);
            else if (value.kind == RHINO_STR)
                output_to(output_string, "%s\n", value.as_str);
            else
                fatal_error("Could not output %s value.", rhino_value_kind_string(value.kind));

            break;
        }

        default:
            fatal_error("Could not interpret %s instruction %d (%02X).", instruction_string(ins), --program_counter, ins);
            break;
        }
    }

#undef NEXT_BYTE
#undef PUSH_STACK
#undef POP_STACK
}
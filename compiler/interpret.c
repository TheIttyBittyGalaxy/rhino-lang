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
        void *as_ptr;
    };
} RhinoValue;

// IO //

char float_to_str_buffer[64];

// FIXME: This implementation cannot handle particularly large floats
//        e.g. 1000000000000000000000000000000.1
void float_to_str(double x)
{
    size_t c = 0;

    if (x < 0)
    {
        float_to_str_buffer[c++] = '-';
        x = -x;
    }

    long long integer_portion = x;
    float rational_portion = x - integer_portion;

    int f = c;
    do
        float_to_str_buffer[c++] = '0' + integer_portion % 10;
    while (integer_portion /= 10);
    int l = c - 1;

    while (l > f)
    {
        char t = float_to_str_buffer[f];
        float_to_str_buffer[f++] = float_to_str_buffer[l];
        float_to_str_buffer[l--] = t;
    }

    if (rational_portion > 0.0001)
    {
        float_to_str_buffer[c++] = '.';
        while (rational_portion > 0.0001)
        {
            int d = rational_portion * 10;
            float_to_str_buffer[c++] = '0' + (d % 10);
            rational_portion = rational_portion * 10 - d;
        }
    }

    float_to_str_buffer[c] = '\0';
}

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

    RhinoValue stack_value[128];
    size_t stack_pointer = 0;

    RhinoValue register_value[256];

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

#define PUSH_STACK(value) stack_value[stack_pointer++] = value
#define POP_STACK() stack_value[--stack_pointer]

    while (program_counter < byte_code->byte_count)
    {
        Instruction ins = (Instruction)NEXT_BYTE();

        // printf("%04X\t%s\n", program_counter - 1, instruction_string(ins));

        switch (ins)
        {

        case JUMP:
        {
            DECODE_BYTES(jump_to, size_t)
            program_counter = jump_to.value;
            break;
        }

        case JUMP_IF_FALSE:
        {
            RhinoValue value = POP_STACK();
            if (value.kind != RHINO_BOOL)
                fatal_error("Could not interpret JUMP_IF_FALSE as value at top of stack is %s.", rhino_value_kind_string(value.kind));

            DECODE_BYTES(jump_to, size_t)
            if (value.as_bool == false)
                program_counter = jump_to.value;

            break;
        }

        case DISCARD_STACK_VALUE:
        {
            POP_STACK();
            break;
        }

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

        case PUSH_REGISTER_VALUE:
        {
            uint8_t reg = NEXT_BYTE();
            PUSH_STACK(register_value[reg]);
            break;
        }

        case SET_REGISTER_VALUE:
        {
            uint8_t reg = NEXT_BYTE();
            register_value[reg] = POP_STACK();
            break;
        }

        case INCREMENT_REGISTER:
        case DECREMENT_REGISTER:
        {
            int diff = ins == INCREMENT_REGISTER ? 1 : -1;
            uint8_t reg = NEXT_BYTE();
            RhinoValue value = register_value[reg];

            if (value.kind == RHINO_INT)
                register_value[reg].as_int += diff;
            else if (value.kind == RHINO_NUM)
                register_value[reg].as_num -= diff;
            else
                fatal_error("Could not interpret %s as value in register %d is %s.", instruction_string(ins), reg, rhino_value_kind_string(value.kind));

            break;
        }

        case PUSH_THEN_INCREMENT_REGISTER:
        case PUSH_THEN_DECREMENT_REGISTER:
        {
            int diff = ins == PUSH_THEN_INCREMENT_REGISTER ? 1 : -1;
            uint8_t reg = NEXT_BYTE();
            RhinoValue value = register_value[reg];

            if (value.kind == RHINO_INT)
                register_value[reg].as_int += diff;
            else if (value.kind == RHINO_NUM)
                register_value[reg].as_num -= diff;
            else
                fatal_error("Could not interpret %s as value in register %d is %s.", instruction_string(ins), reg, rhino_value_kind_string(value.kind));

            PUSH_STACK(value);
            break;
        }

        case OP_NEG:
        {
            RhinoValue value = POP_STACK();

            if (value.kind == RHINO_INT)
                value.as_int = -value.as_int;
            else if (value.kind == RHINO_NUM)
                value.as_num = -value.as_num;
            else
                fatal_error("Could not interpret OP_NEG as value at top of stack is %s.", rhino_value_kind_string(value.kind));

            PUSH_STACK(value);
            break;
        }

        case OP_NOT:
        {
            RhinoValue value = POP_STACK();

            if (value.kind == RHINO_BOOL)
                value.as_bool = !value.as_bool;
            else
                fatal_error("Could not interpret OP_NOT as value at top of stack is %s.", rhino_value_kind_string(value.kind));

            PUSH_STACK(value);
            break;
        }

#define CASE_BINARY_ARITHMETIC(OP, operation)                            \
    case OP:                                                             \
    {                                                                    \
        RhinoValue rhs = POP_STACK();                                    \
        RhinoValue lhs = POP_STACK();                                    \
        RhinoValue result = {.kind = RHINO_NUM};                         \
                                                                         \
        if (lhs.kind == RHINO_INT && rhs.kind == RHINO_INT)              \
        {                                                                \
            result.kind = RHINO_INT;                                     \
            result.as_int = lhs.as_int operation rhs.as_int;             \
        }                                                                \
                                                                         \
        else if (lhs.kind == RHINO_NUM && rhs.kind == RHINO_INT)         \
            result.as_num = lhs.as_num operation rhs.as_int;             \
                                                                         \
        else if (lhs.kind == RHINO_INT && rhs.kind == RHINO_NUM)         \
            result.as_num = lhs.as_int operation rhs.as_num;             \
                                                                         \
        else if (lhs.kind == RHINO_NUM && rhs.kind == RHINO_NUM)         \
            result.as_num = lhs.as_num operation rhs.as_num;             \
                                                                         \
        else                                                             \
            fatal_error(                                                 \
                "Could not interpret " #OP " between %s and %s values.", \
                rhino_value_kind_string(lhs.kind),                       \
                rhino_value_kind_string(rhs.kind));                      \
                                                                         \
        PUSH_STACK(result);                                              \
        break;                                                           \
    }

#define CASE_COMPARE_ARITHMETIC(OP, operation)                           \
    case OP:                                                             \
    {                                                                    \
        RhinoValue rhs = POP_STACK();                                    \
        RhinoValue lhs = POP_STACK();                                    \
        bool result;                                                     \
                                                                         \
        if (lhs.kind == RHINO_INT && rhs.kind == RHINO_INT)              \
            result = lhs.as_int operation rhs.as_int;                    \
                                                                         \
        else if (lhs.kind == RHINO_NUM && rhs.kind == RHINO_INT)         \
            result = lhs.as_num operation rhs.as_int;                    \
                                                                         \
        else if (lhs.kind == RHINO_INT && rhs.kind == RHINO_NUM)         \
            result = lhs.as_int operation rhs.as_num;                    \
                                                                         \
        else if (lhs.kind == RHINO_NUM && rhs.kind == RHINO_NUM)         \
            result = lhs.as_num operation rhs.as_num;                    \
                                                                         \
        else                                                             \
            fatal_error(                                                 \
                "Could not interpret " #OP " between %s and %s values.", \
                rhino_value_kind_string(lhs.kind),                       \
                rhino_value_kind_string(rhs.kind));                      \
                                                                         \
        RhinoValue value = {.kind = RHINO_BOOL, .as_bool = result};      \
        PUSH_STACK(value);                                               \
        break;                                                           \
    }

#define CASE_BINARY_LOGIC(OP, operation)                                 \
    case OP:                                                             \
    {                                                                    \
        RhinoValue rhs = POP_STACK();                                    \
        RhinoValue lhs = POP_STACK();                                    \
                                                                         \
        if (lhs.kind != RHINO_BOOL && rhs.kind == RHINO_BOOL)            \
            fatal_error(                                                 \
                "Could not interpret " #OP " between %s and %s values.", \
                rhino_value_kind_string(lhs.kind),                       \
                rhino_value_kind_string(rhs.kind));                      \
                                                                         \
        bool result = lhs.as_bool operation rhs.as_bool;                 \
        RhinoValue value = {.kind = RHINO_BOOL, .as_bool = result};      \
        PUSH_STACK(value);                                               \
        break;                                                           \
    }

        case OP_DIVIDE:
        {
            RhinoValue rhs = POP_STACK();
            RhinoValue lhs = POP_STACK();
            RhinoValue result = {.kind = RHINO_NUM};

            if (lhs.kind == RHINO_INT && rhs.kind == RHINO_INT)
                result.as_num = (double)lhs.as_int / (double)rhs.as_int;

            else if (lhs.kind == RHINO_NUM && rhs.kind == RHINO_INT)
                result.as_num = lhs.as_num / (double)rhs.as_int;

            else if (lhs.kind == RHINO_INT && rhs.kind == RHINO_NUM)
                result.as_num = (double)lhs.as_int / rhs.as_num;

            else if (lhs.kind == RHINO_NUM && rhs.kind == RHINO_NUM)
                result.as_num = lhs.as_num / rhs.as_num;

            else
                fatal_error(
                    "Could not interpret OP_DIVIDE between %s and %s values.",
                    rhino_value_kind_string(lhs.kind),
                    rhino_value_kind_string(rhs.kind));

            PUSH_STACK(result);
            break;
        }

            CASE_BINARY_ARITHMETIC(OP_MULTIPLY, *)
            CASE_BINARY_ARITHMETIC(OP_ADD, +)
            CASE_BINARY_ARITHMETIC(OP_SUBTRACT, -)

            // TODO: Implement
            // case OP_REMAINDER:

        case OP_EQUAL:
        case OP_NOT_EQUAL:
        {
            RhinoValue rhs = POP_STACK();
            RhinoValue lhs = POP_STACK();
            bool result = false;

            // FIXME: Check this works for all data types
            // FIXME: Implement the correct semantics for strings
            if (lhs.kind == rhs.kind)
            {
                result = lhs.as_ptr == rhs.as_ptr;
            }

            if (ins == OP_NOT_EQUAL)
                result = !result;

            RhinoValue value = {.kind = RHINO_BOOL, .as_bool = result};
            PUSH_STACK(value);
            break;
        }

            CASE_COMPARE_ARITHMETIC(OP_LESS_THAN, <)
            CASE_COMPARE_ARITHMETIC(OP_GREATER_THAN, >)
            CASE_COMPARE_ARITHMETIC(OP_LESS_THAN_EQUAL, <=)
            CASE_COMPARE_ARITHMETIC(OP_GREATER_THAN_EQUAL, >=)

            CASE_BINARY_LOGIC(OP_LOGICAL_AND, &&)
            CASE_BINARY_LOGIC(OP_LOGICAL_OR, ||)

#undef CASE_BINARY_ARITHMETIC
#undef CASE_COMPARE_ARITHMETIC
#undef CASE_BINARY_LOGIC

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
            {
                float_to_str(value.as_num);
                output_to(output_string, "%s\n", float_to_str_buffer);
            }
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
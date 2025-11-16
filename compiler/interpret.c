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

#define NONE_VALUE() (RhinoValue){.kind = RHINO_NONE, .as_bool = false};
#define BOOL_VALUE(value) (RhinoValue){.kind = RHINO_BOOL, .as_bool = value};
#define INT_VALUE(value) (RhinoValue){.kind = RHINO_INT, .as_int = value};
#define NUM_VALUE(value) (RhinoValue){.kind = RHINO_NUM, .as_num = value};
#define STR_VALUE(value) (RhinoValue){.kind = RHINO_STR, .as_str = value};

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

#define FETCH_DATA(T, var)                                      \
    T var;                                                      \
    {                                                           \
        assert(wordsizeof(T) <= 2);                             \
        union                                                   \
        {                                                       \
            T data;                                             \
            uint32_t word[2];                                   \
        } as;                                                   \
                                                                \
        as.word[0] = unit->instruction[program_counter++].word; \
        as.word[1] =                                            \
            (wordsizeof(T) == 2)                                \
                ? unit->instruction[program_counter++].word     \
                : 0;                                            \
                                                                \
        var = as.data;                                          \
    }

void interpret_unit(Unit *unit, RunOnString *output_string)
{
    size_t program_counter = 0;

    RhinoValue stack_value[128];
    size_t stack_pointer = 0;

    RhinoValue register_value[256];

    while (program_counter < unit->count)
    {
        // printf_instruction(unit, program_counter);
        Instruction ins = unit->instruction[program_counter++];

        switch (ins.op)
        {

        case CALL:
        {
            FETCH_DATA(Unit *, data);
            interpret_unit(data, output_string);
            break;
        }

        case JUMP:
            program_counter = ins.x;
            break;

        case JUMP_IF_FALSE:
        {
            RhinoValue value = register_value[ins.a];
            if (value.kind != RHINO_BOOL)
                fatal_error("Could not interpret JUMP_IF_FALSE as value in register is %s.", rhino_value_kind_string(value.kind));

            if (value.as_bool == false)
                program_counter = ins.x;

            break;
        }

        case MOVE:
            register_value[ins.a] = register_value[ins.b];
            break;

        case LOAD_NONE:
            register_value[ins.a] = NONE_VALUE();
            break;

        case LOAD_TRUE:
            register_value[ins.a] = BOOL_VALUE(true);
            break;

        case LOAD_FALSE:
            register_value[ins.a] = BOOL_VALUE(false);
            break;

        case LOAD_INT:
        {
            FETCH_DATA(int, data);
            register_value[ins.a] = INT_VALUE(data);
            break;
        }

        case LOAD_NUM:
        {
            FETCH_DATA(double, data);
            register_value[ins.a] = NUM_VALUE(data);
            break;
        }

        case LOAD_STR:
        {
            FETCH_DATA(char *, data);
            register_value[ins.a] = STR_VALUE(data);
            break;
        }

        case INCREMENT:
        case DECREMENT:
        {
            int diff = ins.op == INCREMENT ? 1 : -1;
            vm_reg reg = ins.a;
            RhinoValue value = register_value[reg];

            if (value.kind == RHINO_INT)
                register_value[reg].as_int += diff;
            else if (value.kind == RHINO_NUM)
                register_value[reg].as_num -= diff;
            else
                fatal_error("Could not interpret %s as value in register %d is %s.", op_code_string((OpCode)ins.op), reg, rhino_value_kind_string(value.kind));

            break;
        }

        case OP_NEG:
        {
            RhinoValue value = register_value[ins.b];

            if (value.kind == RHINO_INT)
                value.as_int = -value.as_int;
            else if (value.kind == RHINO_NUM)
                value.as_num = -value.as_num;
            else
                fatal_error("Could not interpret OP_NEG as value in register is %s.", rhino_value_kind_string(value.kind));

            register_value[ins.a] = value;
            break;
        }

        case OP_NOT:
        {
            RhinoValue value = register_value[ins.b];

            if (value.kind == RHINO_BOOL)
                value.as_bool = !value.as_bool;
            else
                fatal_error("Could not interpret OP_NOT as value in register is %s.", rhino_value_kind_string(value.kind));

            register_value[ins.a] = value;
            break;
        }

#define CASE_BINARY_ARITHMETIC(OP, operation)                            \
    case OP:                                                             \
    {                                                                    \
        RhinoValue lhs = register_value[ins.b];                          \
        RhinoValue rhs = register_value[ins.c];                          \
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
        register_value[ins.a] = result;                                  \
        break;                                                           \
    }

#define CASE_COMPARE_ARITHMETIC(OP, operation)                           \
    case OP:                                                             \
    {                                                                    \
        RhinoValue lhs = register_value[ins.b];                          \
        RhinoValue rhs = register_value[ins.c];                          \
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
        register_value[ins.a] = BOOL_VALUE(result);                      \
        break;                                                           \
    }

#define CASE_BINARY_LOGIC(OP, operation)                                 \
    case OP:                                                             \
    {                                                                    \
        RhinoValue lhs = register_value[ins.b];                          \
        RhinoValue rhs = register_value[ins.c];                          \
                                                                         \
        if (lhs.kind != RHINO_BOOL || rhs.kind != RHINO_BOOL)            \
            fatal_error(                                                 \
                "Could not interpret " #OP " between %s and %s values.", \
                rhino_value_kind_string(lhs.kind),                       \
                rhino_value_kind_string(rhs.kind));                      \
                                                                         \
        bool result = lhs.as_bool operation rhs.as_bool;                 \
        register_value[ins.a] = BOOL_VALUE(result);                      \
        break;                                                           \
    }

        case OP_DIVIDE:
        {
            RhinoValue lhs = register_value[ins.b];
            RhinoValue rhs = register_value[ins.c];
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

            register_value[ins.a] = result;
            break;
        }

        // FIXME: This does not implement Rhino semantics
        case OP_REMAINDER:
        {
            RhinoValue lhs = register_value[ins.b];
            RhinoValue rhs = register_value[ins.c];

            if (lhs.kind != RHINO_INT || rhs.kind != RHINO_INT)
                fatal_error(
                    "Could not interpret OP_REMAINDER between %s and %s values.",
                    rhino_value_kind_string(lhs.kind),
                    rhino_value_kind_string(rhs.kind));

            int result = lhs.as_int % rhs.as_int;
            register_value[ins.a] = INT_VALUE(result);
            break;
        }

            CASE_BINARY_ARITHMETIC(OP_MULTIPLY, *)
            CASE_BINARY_ARITHMETIC(OP_ADD, +)
            CASE_BINARY_ARITHMETIC(OP_SUBTRACT, -)

        case OP_EQUAL:
        case OP_NOT_EQUAL:
        {
            RhinoValue lhs = register_value[ins.b];
            RhinoValue rhs = register_value[ins.c];
            bool result = false;

            // FIXME: Check this works for all data types
            // FIXME: Implement the correct semantics for strings
            if (lhs.kind == rhs.kind)
            {
                result = lhs.as_ptr == rhs.as_ptr;
            }

            if (ins.op == OP_NOT_EQUAL)
                result = !result;

            register_value[ins.a] = BOOL_VALUE(result);
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
            RhinoValue value = register_value[ins.a];
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
            fatal_error("Could not interpret %s instruction %04X.", op_code_string((OpCode)ins.op), --program_counter);
            break;
        }
    }
}

void interpret(ByteCode *byte_code, RunOnString *output_string)
{
    interpret_unit(byte_code->init, output_string);
}
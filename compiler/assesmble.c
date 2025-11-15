#include "assemble.h"

#include "fatal_error.h"

// ASSEMBLER //

typedef struct
{
    void *node;
    uint8_t reg;
} NodeRegister;

typedef struct
{
    const char *source_text;
    Program *apm;

    // TODO: Handle a situation where more than 256 registers are being used
    uint8_t register_count;
    NodeRegister node_to_register[256];
} Assembler;

size_t get_node_register(Assembler *a, void *node)
{
    for (size_t i = 0; i < a->register_count; i++)
        if (a->node_to_register[i].node == node)
            return i;

    fatal_error("Could not get register for APM node %p.", node);
    unreachable;
}

// EMIT BYTES //

#define EMIT(_byte) bc->byte[bc->byte_count++] = (uint8_t)_byte

#define EMIT_DATA(data, T)                 \
    union                                  \
    {                                      \
        T value;                           \
        uint8_t bytes[sizeof(T)];          \
    } __data = {.value = data};            \
    for (size_t i = 0; i < sizeof(T); i++) \
        EMIT(__data.bytes[i]);

// ASSEMBLE EXPRESSION //

uint8_t get_register_of_expression(Assembler *a, Expression *expr)
{
    switch (expr->kind)
    {
    case IDENTITY_LITERAL:
        fatal_error("Attempt to determine the register of an IDENTITY_LITERAL expression. By this pass we would have expected this to become a VARIABLE_REFERENCE.");
        break;

    case VARIABLE_REFERENCE:
        return get_node_register(a, expr->variable);
        break;

    case PARAMETER_REFERENCE:
        return get_node_register(a, expr->parameter);
        break;

        // TODO: Implement
        // case INDEX_BY_FIELD:

    default:
        fatal_error("Attempt to determine the register of a %s expression.", expression_kind_string(expr->kind));
        break;
    }

    unreachable;
}

void assemble_default_value(Assembler *a, ByteCode *bc, RhinoType ty)
{
    Program *apm = a->apm;

    switch (ty.tag)
    {
    case RHINO_NATIVE_TYPE:
        if (IS_BOOL_TYPE(ty))
        {
            EMIT(PUSH_FALSE);
        }
        else if (IS_INT_TYPE(ty))
        {
            EMIT(PUSH_INT);
            EMIT_DATA(0, int);
        }
        else if (IS_NUM_TYPE(ty))
        {
            EMIT(PUSH_INT);
            EMIT_DATA(0, double);
        }
        // else if (IS_STR_TYPE(ty))
        else
        {
            fatal_error("Could not transpile default value for value of native type %s.", ty.native_type->name);
        }

        break;

        // TODO: Implement
        // case RHINO_ENUM_TYPE:

        // TODO: Implement
        // case RHINO_STRUCT_TYPE:

    default:
        fatal_error("Could not transpile default value for value of type %s.", rhino_type_string(apm, ty));
    }
}

void assemble_expression(Assembler *a, ByteCode *bc, Expression *expr)
{
    switch (expr->kind)
    {
    case IDENTITY_LITERAL:
        fatal_error("Attempt to assemble an IDENTITY_LITERAL expression. By this pass we would have expected this to become a VARIABLE_REFERENCE.");
        break;

    case NONE_LITERAL:
        EMIT(PUSH_NONE);
        break;

    case BOOLEAN_LITERAL:
        EMIT(expr->bool_value ? PUSH_TRUE : PUSH_FALSE);
        break;

    case INTEGER_LITERAL:
    {
        EMIT(PUSH_INT);
        EMIT_DATA(expr->integer_value, int);
        break;
    }

    case FLOAT_LITERAL:
    {
        EMIT(PUSH_NUM);
        EMIT_DATA(expr->float_value, double);
        break;
    }

    case STRING_LITERAL:
    {
        // TODO: Do something more efficient than this!!
        substr sub = expr->string_value;
        char *buffer = (char *)malloc(sizeof(char) * (sub.len + 1));
        memcpy(buffer, a->source_text + sub.pos, sub.len);
        buffer[sub.len] = '\0';

        EMIT(PUSH_STR);
        EMIT_DATA(buffer, char *);
        break;
    }

        // TODO: Implement
        // case ENUM_VALUE_LITERAL:

    case VARIABLE_REFERENCE:
        EMIT(PUSH_REGISTER_VALUE);
        EMIT(get_node_register(a, expr->variable));
        break;

        // TODO: Implement
        // case PARAMETER_REFERENCE:

        // TODO: Implement
        // case FUNCTION_CALL:

        // TODO: Implement
        // case INDEX_BY_FIELD:

    case UNARY_POS:
        assemble_expression(a, bc, expr->operand);
        break;

    case UNARY_NEG:
        assemble_expression(a, bc, expr->operand);
        EMIT(OP_NEG);
        break;

    case UNARY_NOT:
        assemble_expression(a, bc, expr->operand);
        EMIT(OP_NOT);
        break;

        // TODO: Implement
        // case UNARY_INCREMENT:
        // case UNARY_DECREMENT:

#define CASE_BINARY(expr_kind, ins)            \
    case expr_kind:                            \
        assemble_expression(a, bc, expr->lhs); \
        assemble_expression(a, bc, expr->rhs); \
        EMIT(ins);                             \
        break;

        CASE_BINARY(BINARY_MULTIPLY, OP_MULTIPLY)
        CASE_BINARY(BINARY_DIVIDE, OP_DIVIDE)
        CASE_BINARY(BINARY_REMAINDER, OP_REMAINDER)
        CASE_BINARY(BINARY_ADD, OP_ADD)
        CASE_BINARY(BINARY_SUBTRACT, OP_SUBTRACT)
        CASE_BINARY(BINARY_LESS_THAN, OP_LESS_THAN)
        CASE_BINARY(BINARY_GREATER_THAN, OP_GREATER_THAN)
        CASE_BINARY(BINARY_LESS_THAN_EQUAL, OP_LESS_THAN_EQUAL)
        CASE_BINARY(BINARY_GREATER_THAN_EQUAL, OP_GREATER_THAN_EQUAL)
        CASE_BINARY(BINARY_EQUAL, OP_EQUAL)
        CASE_BINARY(BINARY_NOT_EQUAL, OP_NOT_EQUAL)
        CASE_BINARY(BINARY_LOGICAL_AND, OP_LOGICAL_AND)
        CASE_BINARY(BINARY_LOGICAL_OR, OP_LOGICAL_OR)

#undef CASE_BINARY

    default:
        fatal_error("Could not assemble %s expression.", expression_kind_string(expr->kind));
        break;
    }
}

// ASSEMBLE CODE BLOCK //

void assemble_code_block(Assembler *a, ByteCode *bc, Block *block)
{
    assert(!block->declaration_block);

    size_t initial_register_count = a->register_count;

    Statement *stmt;
    StatementIterator it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        switch (stmt->kind)
        {

        case FUNCTION_DECLARATION:
        case ENUM_TYPE_DECLARATION:
        case STRUCT_TYPE_DECLARATION:
            break;

        case CODE_BLOCK:
            assemble_code_block(a, bc, stmt->block);
            break;

            // TODO: Implement
            // case ELSE_IF_SEGMENT:
            // case IF_SEGMENT:
            // case ELSE_SEGMENT:

            // TODO: Implement
            // case BREAK_LOOP:
            // case FOR_LOOP:
            // case WHILE_LOOP:
            // case BREAK_STATEMENT:

        case ASSIGNMENT_STATEMENT:
        {
            uint8_t reg = get_register_of_expression(a, stmt->assignment_lhs);
            assemble_expression(a, bc, stmt->assignment_rhs);
            EMIT(SET_REGISTER_VALUE);
            EMIT(reg);
            break;
        }

        case VARIABLE_DECLARATION:
        {
            uint8_t reg = a->register_count++;
            a->node_to_register[reg] = (NodeRegister){
                .node = (void *)stmt->variable,
                .reg = reg};

            if (stmt->initial_value)
                assemble_expression(a, bc, stmt->initial_value);
            else
                assemble_default_value(a, bc, stmt->variable->type);

            EMIT(SET_REGISTER_VALUE);
            EMIT(reg);
            break;
        }

        case OUTPUT_STATEMENT:
            assemble_expression(a, bc, stmt->expression);
            EMIT(OUTPUT_VALUE);
            break;

        case EXPRESSION_STMT:
            assemble_expression(a, bc, stmt->expression);
            break;

            // TODO: Implement
            // case RETURN_STATEMENT:

        default:
            fatal_error("Could not assemble %s statement in code block.", statement_kind_string(stmt->kind));
            break;
        }
    }

    a->register_count = initial_register_count;
}

// ASSEMBLE FUNCTION //

void assemble_function(Assembler *a, ByteCode *bc, Function *funct)
{
    assemble_code_block(a, bc, funct->body);
}

// ASSEMBLE //

void assemble(Compiler *compiler, Program *apm, ByteCode *bc)
{
    Assembler assembler;
    assembler.source_text = compiler->source_text;
    assembler.apm = apm;

    assembler.register_count = 0;

    assemble_function(&assembler, bc, apm->main);
}
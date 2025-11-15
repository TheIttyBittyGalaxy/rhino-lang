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

    size_t loop_depth;
    size_t jump_to_end_of_loop[128][128];
    size_t jump_to_end_of_loop_count[128];
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

// TODO: Turn these into functoins, rather than macros

#define EMIT(_byte) bc->byte[bc->byte_count++] = (uint8_t)_byte

#define EMIT_DATA(data, T)                 \
    union                                  \
    {                                      \
        T value;                           \
        uint8_t bytes[sizeof(T)];          \
    } __data = {.value = data};            \
    for (size_t i = 0; i < sizeof(T); i++) \
        EMIT(__data.bytes[i]);

#define PATCH_DATA(start, data, T)         \
    union                                  \
    {                                      \
        T value;                           \
        uint8_t bytes[sizeof(T)];          \
    } __patch = {.value = data};           \
    size_t __start = start;                \
    for (size_t i = 0; i < sizeof(T); i++) \
        bc->byte[__start + i] = __patch.bytes[i];

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

    case UNARY_INCREMENT:
    {
        uint8_t reg = get_register_of_expression(a, expr->subject);
        EMIT(PUSH_THEN_INCREMENT_REGISTER);
        EMIT(reg);
        break;
    }

    case UNARY_DECREMENT:
    {
        uint8_t reg = get_register_of_expression(a, expr->subject);
        EMIT(PUSH_THEN_DECREMENT_REGISTER);
        EMIT(reg);
        break;
    }

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

    // Track registers in use
    size_t initial_register_count = a->register_count;

    // Assemble statements
    Statement *stmt;
    StatementIterator it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        // Track JUMP instructions to the end of the loop
        if (stmt->kind == FOR_LOOP ||
            stmt->kind == WHILE_LOOP ||
            stmt->kind == BREAK_LOOP)
        {
            a->loop_depth++;
            a->jump_to_end_of_loop_count[a->loop_depth] = 0;
        }

        // Generate the statement
        switch (stmt->kind)
        {

        case FUNCTION_DECLARATION:
        case ENUM_TYPE_DECLARATION:
        case STRUCT_TYPE_DECLARATION:
            break;

        case CODE_BLOCK:
            assemble_code_block(a, bc, stmt->block);
            break;

        case IF_SEGMENT:
        {
            size_t jump_to_end[128];
            size_t jump_to_end_count = 0;

            Statement *segment = stmt;
            while (segment)
            {
                if (segment->kind == ELSE_SEGMENT)
                {
                    assemble_code_block(a, bc, segment->block);
                    break;
                }

                assemble_expression(a, bc, segment->condition);

                EMIT(JUMP_IF_FALSE); // Jump over this segment if the condition fails
                size_t jump_to_next_segment = bc->byte_count;
                EMIT_DATA(0xFFFFFFFF, size_t);

                assemble_code_block(a, bc, segment->body);
                if (segment->next) // Jump to the end of the if statement
                {
                    EMIT(JUMP);
                    jump_to_end[jump_to_end_count++] = bc->byte_count;
                    EMIT_DATA(0xFFFFFFFF, size_t);
                }

                PATCH_DATA(jump_to_next_segment, bc->byte_count, size_t);

                segment = segment->next;
            }

            for (size_t i = 0; i < jump_to_end_count; i++)
            {
                PATCH_DATA(jump_to_end[i], bc->byte_count, size_t);
            }

            break;
        }

        case ELSE_IF_SEGMENT:
        case ELSE_SEGMENT:
            break;

        case BREAK_LOOP:
        {
            size_t jump_to_start = bc->byte_count;
            assemble_code_block(a, bc, stmt->block);
            EMIT(JUMP);
            EMIT_DATA(jump_to_start, size_t);
            break;
        }

            // TODO: Implement
            // case FOR_LOOP:

        case WHILE_LOOP:
        {
            size_t jump_to_start = bc->byte_count;
            assemble_expression(a, bc, stmt->condition);

            EMIT(JUMP_IF_FALSE);
            size_t jump_to_end = bc->byte_count;
            {
                EMIT_DATA(0, size_t);
            }

            assemble_code_block(a, bc, stmt->block);
            EMIT(JUMP);
            {
                EMIT_DATA(jump_to_start, size_t);
            }

            PATCH_DATA(jump_to_end, bc->byte_count, size_t);
            break;
        }

        case BREAK_STATEMENT:
        {
            EMIT(JUMP);
            size_t i = a->jump_to_end_of_loop_count[a->loop_depth]++;
            a->jump_to_end_of_loop[a->loop_depth][i] = bc->byte_count;
            EMIT_DATA(0xFFFFFFFF, size_t);
            break;
        }

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
            EMIT(DISCARD_STACK_VALUE);
            break;

            // TODO: Implement
            // case RETURN_STATEMENT:

        default:
            fatal_error("Could not assemble %s statement in code block.", statement_kind_string(stmt->kind));
            break;
        }

        // Hot patch any jumps to the end of the block
        if (stmt->kind == FOR_LOOP ||
            stmt->kind == WHILE_LOOP ||
            stmt->kind == BREAK_LOOP)
        {
            for (size_t i = 0; i < a->jump_to_end_of_loop_count[a->loop_depth]; i++)
            {
                PATCH_DATA(a->jump_to_end_of_loop[a->loop_depth][i], bc->byte_count, size_t);
            }
            a->loop_depth--;
        }
    }

    // Clear registers
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

    assembler.loop_depth = 0;

    assemble_function(&assembler, bc, apm->main);
}
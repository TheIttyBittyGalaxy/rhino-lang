#include "assemble.h"

#include "fatal_error.h"

#define EMIT(ins) bc->byte[bc->byte_count++] = (uint8_t)ins

// ASSEMBLE EXPRESSION //

void assemble_expression(Compiler *c, Program *apm, ByteCode *bc, Expression *expr)
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
        EMIT(PUSH_INT);
        EMIT(expr->integer_value);
        break;

        // case FLOAT_LITERAL:
        // case STRING_LITERAL:

        // case ENUM_VALUE_LITERAL:
        // case VARIABLE_REFERENCE:
        // case PARAMETER_REFERENCE:

        // case FUNCTION_CALL:

        // case INDEX_BY_FIELD:

    case UNARY_POS:
        assemble_expression(c, apm, bc, expr->operand);
        break;

    case UNARY_NEG:
        assemble_expression(c, apm, bc, expr->operand);
        EMIT(OP_NEG);
        break;

    case UNARY_NOT:
        assemble_expression(c, apm, bc, expr->operand);
        EMIT(OP_NOT);
        break;

        // case UNARY_INCREMENT:
        // case UNARY_DECREMENT:

#define CASE_BINARY(expr_kind, ins)                 \
    case expr_kind:                                 \
        assemble_expression(c, apm, bc, expr->lhs); \
        assemble_expression(c, apm, bc, expr->rhs); \
        EMIT(ins);                                  \
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

void assemble_code_block(Compiler *c, Program *apm, ByteCode *bc, Block *block)
{
    assert(!block->declaration_block);

    Statement *stmt;
    StatementIterator it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        switch (stmt->kind)
        {
        case OUTPUT_STATEMENT:
            assemble_expression(c, apm, bc, stmt->expression);
            EMIT(OUTPUT_VALUE);
            break;

        default:
            fatal_error("Could not assemble %s statement in code block.", statement_kind_string(stmt->kind));
            break;
        }
    }
}

// ASSEMBLE FUNCTION //

void assemble_function(Compiler *c, Program *apm, ByteCode *bc, Function *funct)
{
    assemble_code_block(c, apm, bc, funct->body);
}

// ASSEMBLE //

void assemble(Compiler *c, Program *apm, ByteCode *bc)
{
    assemble_function(c, apm, bc, apm->main);
}
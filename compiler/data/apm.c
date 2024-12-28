#include "apm.h"
#include <stdio.h>

// ENUMS //

DEFINE_ENUM(LIST_EXPRESSIONS, ExpressionKind, expression_kind)
DEFINE_ENUM(LIST_STATEMENTS, StatementKind, statement_kind)

// LIST TYPE //

DEFINE_LIST_TYPE(Expression, expression)
DEFINE_LIST_TYPE(Statement, statement)
DEFINE_LIST_TYPE(Function, function)

DEFINE_SLICE_TYPE(Expression, expression)
DEFINE_SLICE_TYPE(Statement, statement)
DEFINE_SLICE_TYPE(Function, function)

// INIT APM //

void init_program(Program *apm)
{
    init_function_list(&apm->function);
    init_statement_list(&apm->statement);
    init_expression_list(&apm->expression);
}

// DUMP PROGRAM //

void dump_apm(Program *apm, const char *source_text)
{
    printf("FUNCTIONS\n");
    for (size_t i = 0; i < apm->function.count; i++)
    {
        Function *funct = get_function(apm->function, i);
        printf("%02d\t", i);
        printf_substr(source_text, funct->identity);
        printf("\t%02d\t", funct->body);
        printf("\n");
    }
    printf("\n");

    printf("STATEMENTS\n");
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);
        printf("%02d\t%-21s\t", i, statement_kind_string(stmt->kind));
        switch (stmt->kind)
        {
        case CODE_BLOCK:
        case SINGLE_BLOCK:
            printf("first %02d\tlast %02d", stmt->statements.first, stmt->statements.first + stmt->statements.count - 1);
            break;

        case IF_STATEMENT:
        case ELSE_IF_STATEMENT:
            printf("condition %02d\tbody %02d", stmt->condition, stmt->body);
            break;

        case ELSE_STATEMENT:
            printf("body %02d", stmt->body);
            break;

        case OUTPUT_STATEMENT:
        case EXPRESSION_STMT:
            printf("expression %02d", stmt->expression);
            break;
        }
        printf("\n");
    }
    printf("\n");

    printf("EXPRESSIONS\n");
    for (size_t i = 0; i < apm->expression.count; i++)
    {
        Expression *expr = get_expression(apm->expression, i);
        printf("%02d\t%s\t", i, expression_kind_string(expr->kind));
        switch (expr->kind)
        {
        case IDENTITY_LITERAL:
            printf_substr(source_text, expr->identity);
            break;

        case BOOLEAN_LITERAL:
            printf(expr->bool_value ? "true" : "false");
            break;

        case STRING_LITERAL:
            printf_substr(source_text, expr->string_value);
            break;

        case FUNCTION_CALL:
            printf("callee %02d", expr->callee);
            break;
        }
        printf("\n");
    }
}

// PRINT APM //

#define PRINT_PARSED
#include "include/print-apm.c"

#define PRINT_ANALYSED
#include "include/print-apm.c"

// APM UTILITY METHODS //

size_t get_next_statement_in_code_block(Program *apm, Statement *code_block, size_t n)
{
    if (code_block->statements.count == 0)
        return 0;

    Statement *child = get_statement(apm->statement, code_block->statements.first + n);
    n++;

    if (child->kind == CODE_BLOCK || child->kind == SINGLE_BLOCK)
        return n + child->statements.count;
    else if (child->kind == IF_STATEMENT || child->kind == ELSE_IF_STATEMENT || child->kind == ELSE_STATEMENT)
        return n + get_statement(apm->statement, child->body)->statements.count + 1;

    return n;
}

size_t get_first_statement_in_code_block(Program *apm, Statement *code_block)
{
    return 0;
}

size_t get_last_statement_in_code_block(Program *apm, Statement *code_block)
{
    if (code_block->statements.count == 0)
        return 0;

    size_t n = 0;
    while (true)
    {
        size_t next = get_next_statement_in_code_block(apm, code_block, n);
        if (next >= code_block->statements.count)
            return n;
        n = next;
    }
}
#include "apm.h"
#include <stdio.h>

// Enums
DEFINE_ENUM(LIST_EXPRESSIONS, ExpressionKind, expression_kind)
DEFINE_ENUM(LIST_STATEMENTS, StatementKind, statement_kind)

// List type
DEFINE_LIST_TYPE(Expression, expression)
DEFINE_LIST_TYPE(Statement, statement)
DEFINE_LIST_TYPE(Function, function)

DEFINE_SLICE_TYPE(Expression, expression)
DEFINE_SLICE_TYPE(Statement, statement)
DEFINE_SLICE_TYPE(Function, function)

// Init apm
void init_program(Program *apm)
{
    init_function_list(&apm->function);
    init_statement_list(&apm->statement);
    init_expression_list(&apm->expression);
}

// Print apm
size_t newlines_left;
size_t current_indent;
enum LineStatus
{
    NONE,
    LAST,
    ACTIVE
} line_status[64];

#define INDENT() line_status[current_indent++] = ACTIVE
#define UNINDENT() current_indent--
#define LAST_ON_LINE() line_status[current_indent - 1] = LAST

#define BEFORE_PRINT()                                                          \
    {                                                                           \
        if (newlines_left >= 2)                                                 \
        {                                                                       \
            printf("\n");                                                       \
            for (size_t i = 0; i < current_indent; ++i)                         \
                printf(line_status[i] > NONE ? "│   " : "    ");                \
        }                                                                       \
                                                                                \
        if (newlines_left >= 1)                                                 \
        {                                                                       \
            printf("\n");                                                       \
            for (size_t i = 0; i < current_indent - 1; ++i)                     \
                printf(line_status[i] > NONE ? "│   " : "    ");                \
            printf(line_status[current_indent - 1] == ACTIVE ? "├──" : "└───"); \
        }                                                                       \
                                                                                \
        newlines_left = 0;                                                      \
        if (line_status[current_indent - 1] == LAST)                            \
            line_status[current_indent - 1] = NONE;                             \
    }

#define PRINT(...)           \
    {                        \
        BEFORE_PRINT();      \
        printf(__VA_ARGS__); \
    }

#define PRINT_SUBSTR(str)                \
    {                                    \
        BEFORE_PRINT();                  \
        printf_substr(source_text, str); \
    }

#define NEWLINE()          \
    if (newlines_left < 2) \
        newlines_left++;

void print_expression(Program *apm, size_t expr_index, const char *source_text)
{
    Expression *expr = get_expression(apm->expression, expr_index);

    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        PRINT("<INVALID_EXPR>");
        break;

    case BOOLEAN_LITERAL:
        PRINT(expr->bool_value ? "true" : "false");
        break;

    case STRING_LITERAL:
        PRINT_SUBSTR(expr->string_value);
        break;
    }
}

void print_statement(Program *apm, size_t stmt_index, const char *source_text)
{
    Statement *stmt = get_statement(apm->statement, stmt_index);

    PRINT("%s", statement_kind_string(stmt->kind));
    INDENT();
    NEWLINE();

    switch (stmt->kind)
    {
    case CODE_BLOCK:
    case SINGLE_BLOCK:
    {
        for (size_t i = 0; i < stmt->statements.count; i++)
        {
            if (i == stmt->statements.count - 1)
                LAST_ON_LINE(); // FIXME: This will only work when the last statement in the slice is not in a nested scope

            print_statement(apm, stmt->statements.first + i, source_text);

            Statement *child = get_statement(apm->statement, stmt->statements.first + i);
            if (child->kind == CODE_BLOCK || child->kind == SINGLE_BLOCK)
            {
                i += child->statements.count;
            }
            else if (child->kind == IF_STATEMENT)
            {
                Statement *body = get_statement(apm->statement, child->body);
                i += body->statements.count + 1;
            }
        }

        NEWLINE();
        break;
    }

    case IF_STATEMENT:
        PRINT("condition: ");
        print_expression(apm, stmt->condition, source_text);
        NEWLINE();

        LAST_ON_LINE();
        PRINT("body: ");
        print_statement(apm, stmt->body, source_text);
        NEWLINE();

        break;

    case OUTPUT_STATEMENT:
        LAST_ON_LINE();
        print_expression(apm, stmt->value, source_text);

        NEWLINE();
        break;
    }

    UNINDENT();
    NEWLINE();
}

void print_function(Program *apm, size_t funct_index, const char *source_text)
{
    Function *funct = get_function(apm->function, funct_index);

    PRINT("FUNCTION ");
    PRINT_SUBSTR(funct->identity);
    INDENT();
    NEWLINE();

    LAST_ON_LINE();
    PRINT("body: ");
    print_statement(apm, funct->body, source_text);
    UNINDENT();
    NEWLINE();
}

void print_apm(Program *apm, const char *source_text)
{
    newlines_left = 0;
    current_indent = 0;

    for (size_t i = 0; i < apm->function.count; i++)
        print_function(apm, i, source_text);
}

#undef INDENT
#undef UNINDENT
#undef LAST_ON_LINE
#undef BEFORE_PRINT
#undef PRINT
#undef PRINT_SUBSTR
#undef NEWLINE

// Dump program
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
            printf("condition %02d\tbody %02d", stmt->condition, stmt->body);
            break;

        case OUTPUT_STATEMENT:
            printf("value %02d", stmt->value);
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
        case BOOLEAN_LITERAL:
            printf(expr->bool_value ? "true" : "false");
            break;

        case STRING_LITERAL:
            printf_substr(source_text, expr->string_value);
            break;
        }
        printf("\n");
    }
}
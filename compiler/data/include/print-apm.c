#include "../apm.h"
#include <stdio.h>

// STATE //

#ifndef PRINT_APM_STATE
#define PRINT_APM_STATE
size_t newlines_left;
size_t current_indent;
enum LineStatus
{
    NONE,
    LAST,
    ACTIVE
} line_status[64];
#endif

// UTILITY MACROS //

#define INDENT() line_status[current_indent++] = ACTIVE
#define UNINDENT() current_indent--
#define LAST_ON_LINE() line_status[current_indent - 1] = LAST

#define BEFORE_PRINT()                                                               \
    {                                                                                \
        if (newlines_left >= 2)                                                      \
        {                                                                            \
            printf("\n");                                                            \
            for (size_t i = 0; i < current_indent; ++i)                              \
                printf(line_status[i] > NONE ? "│   " : "    ");                     \
        }                                                                            \
                                                                                     \
        if (newlines_left >= 1)                                                      \
        {                                                                            \
            printf("\n");                                                            \
            if (current_indent > 0)                                                  \
            {                                                                        \
                for (size_t i = 0; i < current_indent - 1; ++i)                      \
                    printf(line_status[i] > NONE ? "│   " : "    ");                 \
                printf(line_status[current_indent - 1] == ACTIVE ? "├───" : "└───"); \
            }                                                                        \
        }                                                                            \
                                                                                     \
        newlines_left = 0;                                                           \
        if (line_status[current_indent - 1] == LAST)                                 \
            line_status[current_indent - 1] = NONE;                                  \
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

// FUNCTION NAMES //

#ifdef PRINT_PARSED
#define PRINT_EXPRESSION print_parsed_expression
#define PRINT_STATEMENT print_parsed_statement
#define PRINT_FUNCTION print_parsed_function
#define PRINT_APM print_parsed_apm
#endif

#ifdef PRINT_ANALYSED
#define PRINT_EXPRESSION print_analysed_expression
#define PRINT_STATEMENT print_analysed_statement
#define PRINT_FUNCTION print_analysed_function
#define PRINT_APM print_analysed_apm
#endif

// PRINT EXPRESSION //

void PRINT_EXPRESSION(Program *apm, size_t expr_index, const char *source_text)
{
    Expression *expr = get_expression(apm->expression, expr_index);

    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        PRINT("<INVALID_EXPR>");
        break;

    case IDENTITY_LITERAL:
        PRINT_SUBSTR(expr->identity);
        break;

    case BOOLEAN_LITERAL:
        PRINT(expr->bool_value ? "true" : "false");
        break;

    case STRING_LITERAL:
        PRINT_SUBSTR(expr->string_value);
        break;

    case FUNCTION_CALL:
        PRINT("FUNCTION_CALL");
        INDENT();
        NEWLINE();

        LAST_ON_LINE();
        PRINT("callee: ");
        PRINT_EXPRESSION(apm, expr->callee, source_text);
        NEWLINE();

        UNINDENT();
        NEWLINE();

        break;
    }
}

// PRINT STATEMENT //

void PRINT_STATEMENT(Program *apm, size_t stmt_index, const char *source_text)
{
    Statement *stmt = get_statement(apm->statement, stmt_index);

    if (stmt->kind == EXPRESSION_STMT)
        return PRINT_EXPRESSION(apm, stmt->expression, source_text);

    PRINT("%s", statement_kind_string(stmt->kind));
    INDENT();
    NEWLINE();

    switch (stmt->kind)
    {
    case CODE_BLOCK:
    case SINGLE_BLOCK:
    {
        size_t last = get_last_statement_in_code_block(apm, stmt);
        size_t n = get_first_statement_in_code_block(apm, stmt);
        while (n < stmt->statements.count)
        {
            if (n == last)
                LAST_ON_LINE(); // FIXME: This will only work when the last statement in the slice is not in a nested scope

            PRINT_STATEMENT(apm, stmt->statements.first + n, source_text);
            n = get_next_statement_in_code_block(apm, stmt, n);
        }

        NEWLINE();
        break;
    }

    case IF_STATEMENT:
    case ELSE_IF_STATEMENT:
        PRINT("condition: ");
        PRINT_EXPRESSION(apm, stmt->condition, source_text);
        NEWLINE();

    case ELSE_STATEMENT:
        LAST_ON_LINE();
        PRINT("body: ");
        PRINT_STATEMENT(apm, stmt->body, source_text);
        NEWLINE();

        break;

    case OUTPUT_STATEMENT:
        LAST_ON_LINE();
        PRINT_EXPRESSION(apm, stmt->expression, source_text);

        NEWLINE();
        break;
    }

    UNINDENT();
    NEWLINE();
}

// PRINT FUNCTION //

void PRINT_FUNCTION(Program *apm, size_t funct_index, const char *source_text)
{
    Function *funct = get_function(apm->function, funct_index);

    PRINT("FUNCTION ");
    PRINT_SUBSTR(funct->identity);
    INDENT();
    NEWLINE();

    LAST_ON_LINE();
    PRINT("body: ");
    PRINT_STATEMENT(apm, funct->body, source_text);

    UNINDENT();
    NEWLINE();
}

// PRINT APM //

void PRINT_APM(Program *apm, const char *source_text)
{
    newlines_left = 0;
    current_indent = 0;

#ifdef PRINT_PARSED
    PRINT("PARSED PROGRAM");
#endif
#ifdef PRINT_ANALYSED
    PRINT("ANALYSED PROGRAM");
#endif
    INDENT();
    NEWLINE();

#ifdef PRINT_ANALYSED
    PRINT("main: %d", apm->main);
    NEWLINE();
    NEWLINE();
#endif

    for (size_t i = 0; i < apm->function.count; i++)
    {
        if (i == apm->function.count - 1)
            LAST_ON_LINE();
        PRINT_FUNCTION(apm, i, source_text);
    }

    printf("\n");
}

// UNDEFINE MARCOS //

#undef INDENT
#undef UNINDENT
#undef LAST_ON_LINE
#undef BEFORE_PRINT
#undef PRINT
#undef PRINT_SUBSTR
#undef NEWLINE

#undef PRINT_EXPRESSION
#undef PRINT_STATEMENT
#undef PRINT_FUNCTION
#undef PRINT_APM

#undef PRINT_PARSED
#undef PRINT_ANALYSED
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

#define PRINT_INT(x)     \
    {                    \
        BEFORE_PRINT();  \
        printf("%d", x); \
    }

#define PRINT_FLOAT(x)   \
    {                    \
        BEFORE_PRINT();  \
        printf("%f", x); \
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
#define PRINT_VARIABLE print_parsed_variable
#define PRINT_ENUM_TYPE print_parsed_enum_type
#define PRINT_APM print_parsed_apm
#endif

#ifdef PRINT_RESOLVED
#define PRINT_EXPRESSION print_resolved_expression
#define PRINT_STATEMENT print_resolved_statement
#define PRINT_VARIABLE print_resolved_variable
#define PRINT_ENUM_TYPE print_resolved_enum_type
#define PRINT_APM print_resolved_apm
#endif

// FORWARD DECLARATIONS //

void PRINT_VARIABLE(Program *apm, size_t var_index, const char *source_text);
void PRINT_EXPRESSION(Program *apm, size_t expr_index, const char *source_text);
void PRINT_STATEMENT(Program *apm, size_t stmt_index, const char *source_text);
void PRINT_FUNCTION(Program *apm, size_t funct_index, const char *source_text);
void PRINT_ENUM_TYPE(Program *apm, size_t enum_type_index, const char *source_text);
void PRINT_APM(Program *apm, const char *source_text);

// PRINT VARIABLE //

void PRINT_VARIABLE(Program *apm, size_t var_index, const char *source_text)
{
    Variable *var = get_variable(apm->variable, var_index);

    PRINT_SUBSTR(var->identity);
    PRINT(" v%02d (%s)", var_index, rhino_type_string(apm, var->type));
}

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

    case INTEGER_LITERAL:
        PRINT_INT(expr->integer_value);
        break;

    case FLOAT_LITERAL:
        PRINT_FLOAT(expr->float_value);
        break;

    case BOOLEAN_LITERAL:
        PRINT(expr->bool_value ? "true" : "false");
        break;

    case STRING_LITERAL:
        PRINT("\"");
        PRINT_SUBSTR(expr->string_value);
        PRINT("\"");
        break;

    case ENUM_VALUE_LITERAL:
    {
        EnumValue *enum_value = get_enum_value(apm->enum_value, expr->enum_value);
        PRINT_SUBSTR(enum_value->identity);
        break;
    }

    case VARIABLE_REFERENCE:
        PRINT("v%02d", expr->variable);
        break;

    case TYPE_REFERENCE:
        PRINT("t%s", rhino_type_string(apm, expr->type));
        break;

    case FUNCTION_CALL:
        PRINT("FUNCTION_CALL");
        INDENT();
        NEWLINE();

        LAST_ON_LINE();

#ifdef PRINT_PARSED
        PRINT("callee: ");
        PRINT_EXPRESSION(apm, expr->callee, source_text);
#endif
#ifdef PRINT_RESOLVED
        PRINT("callee: %d", expr->callee);
#endif
        NEWLINE();

        UNINDENT();
        NEWLINE();

        break;

    case INDEX_BY_FIELD:
        PRINT("INDEX_BY_FIELD ");
        INDENT();

        NEWLINE();
        PRINT("field: ")
        PRINT_SUBSTR(expr->field);

        NEWLINE();
        LAST_ON_LINE();
        PRINT("subject: ")
        PRINT_EXPRESSION(apm, expr->subject, source_text);

        UNINDENT();
        NEWLINE();

        break;

    case RANGE_LITERAL:
        PRINT("RANGE");
        INDENT();

        NEWLINE();
        PRINT("first: ")
        PRINT_EXPRESSION(apm, expr->first, source_text);

        NEWLINE();
        LAST_ON_LINE();
        PRINT("last: ")
        PRINT_EXPRESSION(apm, expr->last, source_text);

        UNINDENT();
        NEWLINE();

        break;

    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        PRINT(expression_kind_string(expr->kind));
        INDENT();

        NEWLINE();
        LAST_ON_LINE();
        PRINT_EXPRESSION(apm, expr->operand, source_text);

        UNINDENT();
        NEWLINE();

        break;

    case BINARY_MULTIPLY:
    case BINARY_DIVIDE:
    case BINARY_REMAINDER:
    case BINARY_ADD:
    case BINARY_SUBTRACT:
    case BINARY_LESS_THAN:
    case BINARY_GREATER_THAN:
    case BINARY_LESS_THAN_EQUAL:
    case BINARY_GREATER_THAN_EQUAL:
    case BINARY_EQUAL:
    case BINARY_NOT_EQUAL:
    case BINARY_LOGICAL_AND:
    case BINARY_LOGICAL_OR:
        PRINT(expression_kind_string(expr->kind));
        INDENT();

        NEWLINE();
        PRINT_EXPRESSION(apm, expr->lhs, source_text);

        NEWLINE();
        LAST_ON_LINE();
        PRINT_EXPRESSION(apm, expr->rhs, source_text);

        UNINDENT();
        NEWLINE();

        break;

    default:
        fatal_error("Could not pretty print %s expression", expression_kind_string(expr->kind));
    }
}

// PRINT STATEMENT //

void PRINT_STATEMENT(Program *apm, size_t stmt_index, const char *source_text)
{
    Statement *stmt = get_statement(apm->statement, stmt_index);

    if (stmt->kind == EXPRESSION_STMT)
    {
        PRINT_EXPRESSION(apm, stmt->expression, source_text);
        NEWLINE();
        NEWLINE();
        return;
    }

    if (stmt->kind == DECLARATION_BLOCK)
    {
        size_t last = get_last_statement_in_code_block(apm, stmt);
        size_t n = get_first_statement_in_code_block(apm, stmt);
        while (n < stmt->statements.count)
        {
            if (n == last)
                LAST_ON_LINE();

            PRINT_STATEMENT(apm, stmt->statements.first + n, source_text);
            n = get_next_statement_in_code_block(apm, stmt, n);
        }

        NEWLINE();
        return;
    }

    if (stmt->kind == FUNCTION_DECLARATION)
    {
        PRINT_FUNCTION(apm, stmt->function, source_text);
        return;
    }

    if (stmt->kind == ENUM_TYPE_DECLARATION)
    {
        PRINT_ENUM_TYPE(apm, stmt->enum_type, source_text);
        return;
    }

    PRINT("%s", statement_kind_string(stmt->kind));
    INDENT();
    NEWLINE();

    switch (stmt->kind)
    {
    case INVALID_STATEMENT:
        PRINT("<INVALID_STMT>");
        break;

    case CODE_BLOCK:
    case SINGLE_BLOCK:
    {
        size_t last = get_last_statement_in_code_block(apm, stmt);
        size_t n = get_first_statement_in_code_block(apm, stmt);

        if (stmt->statements.count > 1)
            NEWLINE();

        while (n < stmt->statements.count)
        {
            if (n == last)
                LAST_ON_LINE();

            PRINT_STATEMENT(apm, stmt->statements.first + n, source_text);
            n = get_next_statement_in_code_block(apm, stmt, n);
        }

        NEWLINE();
        break;
    }

    case IF_SEGMENT:
    case ELSE_IF_SEGMENT:
        PRINT("condition: ");
        PRINT_EXPRESSION(apm, stmt->condition, source_text);
        NEWLINE();

    case ELSE_SEGMENT:
        LAST_ON_LINE();
        PRINT("body: ");
        PRINT_STATEMENT(apm, stmt->body, source_text);
        NEWLINE();

        break;

    case BREAK_LOOP:
        break;

    case FOR_LOOP:
        PRINT("iterator: ");
        PRINT_VARIABLE(apm, stmt->iterator, source_text);
        NEWLINE();

        PRINT("iterable: ");
        PRINT_EXPRESSION(apm, stmt->iterable, source_text);
        NEWLINE();

        LAST_ON_LINE();
        PRINT("body: ");
        PRINT_STATEMENT(apm, stmt->body, source_text);
        NEWLINE();

        break;

    case ASSIGNMENT_STATEMENT:
        PRINT("lhs: ");
        PRINT_EXPRESSION(apm, stmt->assignment_lhs, source_text);
        NEWLINE();

        LAST_ON_LINE();
        PRINT("rhs: ");
        PRINT_EXPRESSION(apm, stmt->assignment_rhs, source_text);
        NEWLINE();

        break;

    case VARIABLE_DECLARATION:
        if (!stmt->has_type_expression && !stmt->has_initial_value)
            LAST_ON_LINE();

        PRINT("variable: ");
        PRINT_VARIABLE(apm, stmt->variable, source_text);
        NEWLINE();

        if (stmt->has_type_expression)
        {
            if (!stmt->has_initial_value)
                LAST_ON_LINE();

            PRINT("type_expression: ");
            PRINT_EXPRESSION(apm, stmt->type_expression, source_text);
            NEWLINE();
        }

        if (stmt->has_initial_value)
        {
            LAST_ON_LINE();
            PRINT("initial_value: ");
            PRINT_EXPRESSION(apm, stmt->initial_value, source_text);
            NEWLINE();
        }

        break;

    case OUTPUT_STATEMENT:
        LAST_ON_LINE();
        PRINT_EXPRESSION(apm, stmt->expression, source_text);
        NEWLINE();

        break;

    case RETURN_STATEMENT:
        LAST_ON_LINE();
        PRINT_EXPRESSION(apm, stmt->expression, source_text);
        NEWLINE();

        break;

    default:
        fatal_error("Could not pretty print %s statement", statement_kind_string(stmt->kind));
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

#ifdef PRINT_PARSED
    PRINT("has_return_type_expression: %s", funct->has_return_type_expression ? "true" : "false");
    NEWLINE();

    if (funct->has_return_type_expression)
    {
        PRINT("return_type_expression: ");
        PRINT_EXPRESSION(apm, funct->return_type_expression, source_text);
        NEWLINE();
    }

#endif
#ifdef PRINT_RESOLVED
    PRINT("return_type: %s", rhino_type_string(apm, funct->return_type));
    NEWLINE();

#endif

    LAST_ON_LINE();
    PRINT("body: ");
    PRINT_STATEMENT(apm, funct->body, source_text);

    UNINDENT();
    NEWLINE();
}

// PRINT ENUM //

void PRINT_ENUM_TYPE(Program *apm, size_t enum_type_index, const char *source_text)
{
    EnumType *enum_type = get_enum_type(apm->enum_type, enum_type_index);

    PRINT("ENUM ");
    PRINT_SUBSTR(enum_type->identity);

    if (enum_type->values.count > 0)
    {
        INDENT();
        NEWLINE();
        size_t last = enum_type->values.first + enum_type->values.count - 1;
        for (size_t i = enum_type->values.first; i <= last; i++)
        {
            EnumValue *enum_value = get_enum_value(apm->enum_value, i);
            if (i == last)
                LAST_ON_LINE();
            PRINT_SUBSTR(enum_value->identity);
            NEWLINE();
        }
        UNINDENT();
    }
    NEWLINE();
}

// PRINT APM //

void PRINT_APM(Program *apm, const char *source_text)
{
    newlines_left = 0;
    current_indent = 0;

#ifdef PRINT_PARSED
    PRINT("PARSED PROGRAM");
    INDENT();
#endif
#ifdef PRINT_RESOLVED
    PRINT("RESOLVED PROGRAM");
    INDENT();

    NEWLINE();
    PRINT("main: %d", apm->main);
#endif

    NEWLINE();
    NEWLINE();
    PRINT_STATEMENT(apm, apm->program_block, source_text);
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
#undef PRINT_VARIABLE
#undef PRINT_ENUM_TYPE
#undef PRINT_APM

#undef PRINT_PARSED
#undef PRINT_RESOLVED

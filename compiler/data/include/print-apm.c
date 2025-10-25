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
#define PRINT_BLOCK print_parsed_block
#define PRINT_FUNCTION print_parsed_function
#define PRINT_VARIABLE print_parsed_variable
#define PRINT_ENUM_TYPE print_parsed_enum_type
#define PRINT_STRUCT_TYPE print_parsed_struct_type
#define PRINT_APM print_parsed_apm
#endif

#ifdef PRINT_RESOLVED
#define PRINT_EXPRESSION print_resolved_expression
#define PRINT_STATEMENT print_resolved_statement
#define PRINT_BLOCK print_resolved_block
#define PRINT_VARIABLE print_resolved_variable
#define PRINT_ENUM_TYPE print_resolved_enum_type
#define PRINT_STRUCT_TYPE print_resolved_struct_type
#define PRINT_APM print_resolved_apm
#endif

// FORWARD DECLARATIONS //

void PRINT_VARIABLE(Program *apm, Variable *var, const char *source_text);
void PRINT_EXPRESSION(Program *apm, Expression *expr, const char *source_text);
void PRINT_STATEMENT(Program *apm, Statement *stmt, const char *source_text);
void PRINT_BLOCK(Program *apm, Block *block, const char *source_text);
void PRINT_FUNCTION(Program *apm, Function *funct, const char *source_text);
void PRINT_ENUM_TYPE(Program *apm, EnumType *enum_type, const char *source_text);
void PRINT_STRUCT_TYPE(Program *apm, StructType *struct_type, const char *source_text);
void PRINT_APM(Program *apm, const char *source_text);

// PRINT VARIABLE //

void PRINT_VARIABLE(Program *apm, Variable *var, const char *source_text)
{
    PRINT_SUBSTR(var->identity);
    PRINT(" v%p (%s)", var, rhino_type_string(apm, var->type));
}

// PRINT EXPRESSION //

void PRINT_EXPRESSION(Program *apm, Expression *expr, const char *source_text)
{
    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        PRINT("<INVALID_EXPR>");
        break;

    case IDENTITY_LITERAL:
        PRINT_SUBSTR(expr->identity);
        break;

    case NONE_LITERAL:
        PRINT("none");
        break;

    case BOOLEAN_LITERAL:
        PRINT(expr->bool_value ? "true" : "false");
        break;

    case INTEGER_LITERAL:
        PRINT_INT(expr->integer_value);
        break;

    case FLOAT_LITERAL:
        PRINT_FLOAT(expr->float_value);
        break;

    case STRING_LITERAL:
        PRINT("\"");
        PRINT_SUBSTR(expr->string_value);
        PRINT("\"");
        break;

    case ENUM_VALUE_LITERAL:
    {
        PRINT_SUBSTR(expr->enum_value->identity);
        break;
    }

    case VARIABLE_REFERENCE:
        PRINT("v<%02d>", expr->variable);
        break;

    case TYPE_REFERENCE:
        PRINT("t%s", rhino_type_string(apm, expr->type));
        break;

    case FUNCTION_REFERENCE:
        PRINT("f<%02d>", expr->function);
        break;

    case PARAMETER_REFERENCE:
        PRINT("p<%02d>", expr->parameter);
        break;

    case FUNCTION_CALL:
        PRINT("FUNCTION_CALL");
        INDENT();
        NEWLINE();

        if (expr->arguments.count == 0)
            LAST_ON_LINE();

        PRINT("callee: ");
        PRINT_EXPRESSION(apm, expr->callee, source_text);
        NEWLINE();

        if (expr->arguments.count > 0)
        {
            LAST_ON_LINE();
            PRINT("arguments:");
            NEWLINE();
            INDENT();

            Argument *arg;
            ArgumentIterator it = argument_iterator(expr->arguments);
            size_t i = 0;
            while (arg = next_argument_iterator(&it))
            {
                if (i == expr->arguments.count - 1)
                    LAST_ON_LINE();

                PRINT_EXPRESSION(apm, arg->expr, source_text);
                NEWLINE();

                i++;
            }

            UNINDENT();
        }

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

    case NONEABLE_EXPRESSION:
        PRINT("(")
        PRINT_EXPRESSION(apm, expr->subject, source_text);
        PRINT(")?")

        break;

    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_NOT:
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

void PRINT_STATEMENT(Program *apm, Statement *stmt, const char *source_text)
{
    if (stmt->kind == EXPRESSION_STMT)
    {
        PRINT_EXPRESSION(apm, stmt->expression, source_text);
        NEWLINE();
        NEWLINE();
        return;
    }

    if (stmt->kind == CODE_BLOCK)
    {
        PRINT_BLOCK(apm, stmt->block, source_text);
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

    if (stmt->kind == STRUCT_TYPE_DECLARATION)
    {
        PRINT_STRUCT_TYPE(apm, stmt->struct_type, source_text);
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

    case IF_SEGMENT:
    case ELSE_IF_SEGMENT:
        PRINT("condition: ");
        PRINT_EXPRESSION(apm, stmt->condition, source_text);
        NEWLINE();

    case ELSE_SEGMENT:
        LAST_ON_LINE();
        PRINT("body: ");
        PRINT_BLOCK(apm, stmt->body, source_text);
        NEWLINE();

        break;

    case BREAK_LOOP:
        LAST_ON_LINE();
        PRINT("body: ");
        PRINT_BLOCK(apm, stmt->body, source_text);
        NEWLINE();

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
        PRINT_BLOCK(apm, stmt->body, source_text);
        NEWLINE();

        break;

    case WHILE_LOOP:
        PRINT("condition: ");
        PRINT_EXPRESSION(apm, stmt->condition, source_text);
        NEWLINE();

        LAST_ON_LINE();
        PRINT("body: ");
        PRINT_BLOCK(apm, stmt->body, source_text);
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
        if (!stmt->has_valid_identity)
        {
            PRINT("has_valid_identity: false");
            NEWLINE();
        }

        PRINT("variable: ");
        PRINT_VARIABLE(apm, stmt->variable, source_text);
        NEWLINE();

        if (stmt->type_expression == NULL && stmt->initial_value == NULL)
            LAST_ON_LINE();

        PRINT("order: %d", stmt->variable->order);
        NEWLINE();

        if (stmt->type_expression)
        {
            if (stmt->initial_value == NULL)
                LAST_ON_LINE();

            PRINT("type_expression: ");
            PRINT_EXPRESSION(apm, stmt->type_expression, source_text);
            NEWLINE();
        }

        if (stmt->initial_value)
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
        if (stmt->expression)
        {
            LAST_ON_LINE();
            PRINT_EXPRESSION(apm, stmt->expression, source_text);
            NEWLINE();
        }

        break;

    default:
        fatal_error("Could not pretty print %s statement", statement_kind_string(stmt->kind));
    }

    UNINDENT();
    NEWLINE();
}

// PRINT BLOCK //

void PRINT_BLOCK(Program *apm, Block *block, const char *source_text)
{
    PRINT(block->declaration_block ? "DECLARATIONS" : "BLOCK");
    INDENT();
    NEWLINE();

    Statement *stmt;
    StatementIterator it = statement_iterator(block->statements);
    size_t i = 0;
    while (stmt = next_statement_iterator(&it))
    {
        if (i == block->statements.count - 1)
            LAST_ON_LINE();

        PRINT_STATEMENT(apm, stmt, source_text);
        i++;
    }

    UNINDENT();
}

// PRINT FUNCTION //

void PRINT_FUNCTION(Program *apm, Function *funct, const char *source_text)
{
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

    if (funct->parameters.count > 0)
    {
        PRINT("parameters:");
        NEWLINE();
        INDENT();

        Parameter *parameter;
        ParameterIterator it = parameter_iterator(funct->parameters);
        size_t i = 0;
        while (parameter = next_parameter_iterator(&it))
        {
            if (i == funct->parameters.count - 1)
                LAST_ON_LINE();

#ifdef PRINT_PARSED
            PRINT_EXPRESSION(apm, parameter->type_expression, source_text);
            PRINT(" ");
#endif
#ifdef PRINT_RESOLVED
            PRINT("%s ", rhino_type_string(apm, parameter->type));
#endif

            PRINT_SUBSTR(parameter->identity);
            NEWLINE();
        }
        UNINDENT();
    }

    LAST_ON_LINE();
    PRINT("body: ");
    PRINT_BLOCK(apm, funct->body, source_text);

    UNINDENT();
    NEWLINE();
}

// PRINT ENUM //

void PRINT_ENUM_TYPE(Program *apm, EnumType *enum_type, const char *source_text)
{
    PRINT("ENUM ");
    PRINT_SUBSTR(enum_type->identity);

    if (enum_type->values.count > 0)
    {
        INDENT();
        NEWLINE();

        EnumValue *enum_value;
        EnumValueIterator it = enum_value_iterator(enum_type->values);
        size_t i = 0;
        while (enum_value = next_enum_value_iterator(&it))
        {
            if (i == enum_type->values.count - 1)
                LAST_ON_LINE();

            PRINT_SUBSTR(enum_value->identity);
            NEWLINE();

            i++;
        }
        UNINDENT();
    }
    NEWLINE();
}

// PRINT STRUCT //

void PRINT_STRUCT_TYPE(Program *apm, StructType *struct_type, const char *source_text)
{
    PRINT("STRUCT ");
    PRINT_SUBSTR(struct_type->identity);
    INDENT();

    Block *declarations = struct_type->body;
    if (declarations->statements.count > 0)
    {
        NEWLINE();
        PRINT_BLOCK(apm, declarations, source_text);
    }

    if (struct_type->properties.count > 0)
    {
        NEWLINE();

        Property *property;
        PropertyIterator it = property_iterator(struct_type->properties);
        size_t i = 0;
        while (property = next_property_iterator(&it))
        {
            if (i == struct_type->properties.count - 1)
                LAST_ON_LINE();

            PRINT("%s ", rhino_type_string(apm, property->type));
            PRINT_SUBSTR(property->identity);
            NEWLINE();
            i++;
        }
    }

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
    Statement *stmt;
    StatementIterator it = statement_iterator(apm->program_block->statements);
    size_t i = 0;
    while (stmt = next_statement_iterator(&it))
    {
        if (i == apm->program_block->statements.count - 1)
            LAST_ON_LINE();

        PRINT_STATEMENT(apm, stmt, source_text);
        i++;
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
#undef PRINT_BLOCK
#undef PRINT_FUNCTION
#undef PRINT_VARIABLE
#undef PRINT_ENUM_TYPE
#undef PRINT_STRUCT_TYPE
#undef PRINT_APM

#undef PRINT_PARSED
#undef PRINT_RESOLVED

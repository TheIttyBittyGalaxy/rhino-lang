#include "interpret.h"
#include <stdio.h>

// Values
#define LIST_VALUE_TYPES(MACRO) \
    MACRO(INVALID_VALUE_TYPE)   \
                                \
    MACRO(TYPE_STRING)          \
    MACRO(TYPE_BOOLEAN)

DECLARE_ENUM(LIST_VALUE_TYPES, ValueType, value_type)
DEFINE_ENUM(LIST_VALUE_TYPES, ValueType, value_type)

typedef struct
{
    ValueType type;
    int value;
} Value;

// Interpret
Value interpret_expression(Program *apm, size_t expr_index)
{
    Expression *expr = get_expression(apm->expression, expr_index);
    Value result;

    switch (expr->kind)
    {
    case BOOLEAN_LITERAL:
        result.type = TYPE_BOOLEAN;
        result.value = expr->bool_value ? 1 : 0;
        break;

    case STRING_LITERAL:
        result.type = TYPE_STRING;
        result.value = expr_index;
        break;

    default:
        // TODO: Error
        break;
    }

    return result;
}

void interpret_statement(Program *apm, const char *source_text, size_t stmt_index)
{
    Statement *stmt = get_statement(apm->statement, stmt_index);

    switch (stmt->kind)
    {
    case CODE_BLOCK:
    case SINGLE_BLOCK:
    {
        for (size_t n = 0; n < stmt->statements.count; n++)
        {
            size_t i = stmt->statements.first + n;
            // print("%d\t%d\n", stmt->statements.first, i);
            interpret_statement(apm, source_text, i);

            Statement *child = get_statement(apm->statement, i);
            if (child->kind == CODE_BLOCK || child->kind == SINGLE_BLOCK)
            {
                n += child->statements.count;
            }
            else if (child->kind == IF_STATEMENT)
            {
                Statement *body = get_statement(apm->statement, child->body);
                n += body->statements.count + 1;
            }
        }
        break;
    }

    case IF_STATEMENT:
    {
        Value condition = interpret_expression(apm, stmt->condition);
        if (condition.value)
            interpret_statement(apm, source_text, stmt->body);
        break;
    }

    case OUTPUT_STATEMENT:
    {
        Value result = interpret_expression(apm, stmt->value);
        if (result.type == TYPE_STRING)
        {
            Expression *literal = get_expression(apm->expression, result.value);
            printf_substr(source_text, literal->string_value);
            printf("\n");
        }
        else if (result.type == TYPE_BOOLEAN)
        {
            printf("%s\n", result.value ? "true" : "false");
        }
        else
        {
            // TODO: Error
        }

        break;
    }

    default:
        // TODO: Error
        break;
    }
}

void interpret_function(Program *apm, const char *source_text, size_t funct_index)
{
    Function *funct = get_function(apm->function, funct_index);
    interpret_statement(apm, source_text, funct->body);
}

void interpret(Program *apm, const char *source_text)
{
    // FIXME: Call main, not the first function
    interpret_function(apm, source_text, 0);
}
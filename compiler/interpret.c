#include "interpret.h"
#include <stdio.h>

// Interpreter
typedef struct
{
    bool executed_if_segment;
} StackLayer;

typedef struct
{
    StackLayer *stack_layer;
    size_t stack_layer_count;
    size_t stack_layer_capacity;
} Interpreter;

#define STACK_LAYER interpreter->stack_layer[interpreter->stack_layer_count]

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
Value interpret_expression(Interpreter *interpreter, Program *apm, size_t expr_index)
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

void interpret_statement(Interpreter *interpreter, Program *apm, const char *source_text, size_t stmt_index)
{
    Statement *stmt = get_statement(apm->statement, stmt_index);

    switch (stmt->kind)
    {
    case CODE_BLOCK:
    case SINGLE_BLOCK:
    {
        if (interpreter->stack_layer_count == interpreter->stack_layer_capacity)
        {
            interpreter->stack_layer_capacity = interpreter->stack_layer_capacity * 2;
            interpreter->stack_layer = (StackLayer *)realloc(interpreter->stack_layer, sizeof(StackLayer) * interpreter->stack_layer_capacity);
        }

        STACK_LAYER.executed_if_segment = false;
        interpreter->stack_layer_count++;

        size_t n = get_first_statement_in_code_block(apm, stmt);
        while (n < stmt->statements.count)
        {
            interpret_statement(interpreter, apm, source_text, stmt->statements.first + n);
            n = get_next_statement_in_code_block(apm, stmt, n);
        }

        interpreter->stack_layer_count--;
        break;
    }

    case IF_STATEMENT:
    {
        STACK_LAYER.executed_if_segment = false;
        Value condition = interpret_expression(interpreter, apm, stmt->condition);
        if (condition.value)
        {
            interpret_statement(interpreter, apm, source_text, stmt->body);
            STACK_LAYER.executed_if_segment = true;
        }
        break;
    }

    case ELSE_IF_STATEMENT:
    {
        if (!STACK_LAYER.executed_if_segment)
        {
            Value condition = interpret_expression(interpreter, apm, stmt->condition);
            if (condition.value)
            {
                interpret_statement(interpreter, apm, source_text, stmt->body);
                STACK_LAYER.executed_if_segment = true;
            }
        }
        break;
    }

    case ELSE_STATEMENT:
    {
        if (!STACK_LAYER.executed_if_segment)
        {
            interpret_statement(interpreter, apm, source_text, stmt->body);
            STACK_LAYER.executed_if_segment = true;
        }
        break;
    }

    case OUTPUT_STATEMENT:
    {
        Value result = interpret_expression(interpreter, apm, stmt->value);
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

void interpret_function(Interpreter *interpreter, Program *apm, const char *source_text, size_t funct_index)
{
    Function *funct = get_function(apm->function, funct_index);
    interpret_statement(interpreter, apm, source_text, funct->body);
}

void interpret(Program *apm, const char *source_text)
{
    Interpreter interpreter;
    interpreter.stack_layer_capacity = 1;
    interpreter.stack_layer_count = 0;
    interpreter.stack_layer = (StackLayer *)malloc(sizeof(StackLayer) * interpreter.stack_layer_capacity);

    // FIXME: Call main, not the first function
    interpret_function(&interpreter, apm, source_text, 0);

    free(interpreter.stack_layer);
}
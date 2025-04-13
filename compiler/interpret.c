#include "interpret.h"
#include "fatal_error.h"
#include <stdio.h>

// Interpreter
typedef struct
{
    bool already_executed_if_segment;
} StackLayer;

typedef struct
{
    const char *source_text;

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
    MACRO(TYPE_BOOLEAN)         \
    MACRO(TYPE_NUMBER)

DECLARE_ENUM(LIST_VALUE_TYPES, ValueType, value_type)
DEFINE_ENUM(LIST_VALUE_TYPES, ValueType, value_type)

typedef struct
{
    ValueType type;
    int value;
} Value;

// Interpret
Value interpret_expression(Interpreter *interpreter, Program *apm, size_t expr_index);
void interpret_statement(Interpreter *interpreter, Program *apm, size_t stmt_index);
void interpret_function(Interpreter *interpreter, Program *apm, size_t funct_index);

Value interpret_expression(Interpreter *interpreter, Program *apm, size_t expr_index)
{
    Expression *expr = get_expression(apm->expression, expr_index);
    Value result;

    switch (expr->kind)
    {
        // TODO: Implement
        // case IDENTITY_LITERAL:

    case BOOLEAN_LITERAL:
        result.type = TYPE_BOOLEAN;
        result.value = expr->bool_value ? 1 : 0;
        break;

    case NUMBER_LITERAL:
        result.type = TYPE_NUMBER;
        result.value = expr->number_value;
        break;

    case STRING_LITERAL:
        result.type = TYPE_STRING;
        result.value = expr_index;
        break;

    case FUNCTION_CALL:
        interpret_function(interpreter, apm, expr->callee);
        break;

    default:
        fatal_error("Could not interpret %s expression.", expression_kind_string(expr->kind));
        break;
    }

    return result;
}

void interpret_statement(Interpreter *interpreter, Program *apm, size_t stmt_index)
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

        STACK_LAYER.already_executed_if_segment = false;
        interpreter->stack_layer_count++;

        size_t n = get_first_statement_in_code_block(apm, stmt);
        while (n < stmt->statements.count)
        {
            interpret_statement(interpreter, apm, stmt->statements.first + n);
            n = get_next_statement_in_code_block(apm, stmt, n);
        }

        interpreter->stack_layer_count--;
        break;
    }

    case IF_SEGMENT:
    {
        STACK_LAYER.already_executed_if_segment = false;
        Value condition = interpret_expression(interpreter, apm, stmt->condition);
        if (condition.value)
        {
            interpret_statement(interpreter, apm, stmt->body);
            STACK_LAYER.already_executed_if_segment = true;
        }
        break;
    }

    case ELSE_IF_SEGMENT:
    {
        if (STACK_LAYER.already_executed_if_segment)
            break;

        Value condition = interpret_expression(interpreter, apm, stmt->condition);
        if (condition.value)
        {
            interpret_statement(interpreter, apm, stmt->body);
            STACK_LAYER.already_executed_if_segment = true;
        }
        break;
    }

    case ELSE_SEGMENT:
    {
        if (STACK_LAYER.already_executed_if_segment)
            break;

        interpret_statement(interpreter, apm, stmt->body);
        STACK_LAYER.already_executed_if_segment = true;
        break;
    }

        // TODO: Implement
        // case ASSIGNMENT_STATEMENT:

    case VARIABLE_DECLARATION:
        // Nothing needs to happen
        break;

    case OUTPUT_STATEMENT:
    {
        Value result = interpret_expression(interpreter, apm, stmt->expression);
        if (result.type == TYPE_STRING)
        {
            Expression *literal = get_expression(apm->expression, result.value);
            printf_substr(interpreter->source_text, literal->string_value);
            printf("\n");
        }
        else if (result.type == TYPE_NUMBER)
        {
            printf("%d\n", result.value);
        }
        else if (result.type == TYPE_BOOLEAN)
        {
            printf("%s\n", result.value ? "true" : "false");
        }
        else
        {
            fatal_error("Could not interpret %s statement with %s expression.", statement_kind_string(stmt->kind), value_type_string(result.type));
        }

        break;
    }

    case EXPRESSION_STMT:
    {
        interpret_expression(interpreter, apm, stmt->expression);
        break;
    }

    default:
        fatal_error("Could not interpret %s statement.", statement_kind_string(stmt->kind));
        break;
    }
}

void interpret_function(Interpreter *interpreter, Program *apm, size_t funct_index)
{
    Function *funct = get_function(apm->function, funct_index);
    interpret_statement(interpreter, apm, funct->body);
}

void interpret(Program *apm, const char *source_text)
{
    Interpreter interpreter;
    interpreter.source_text = source_text;

    interpreter.stack_layer_capacity = 1;
    interpreter.stack_layer_count = 0;
    interpreter.stack_layer = (StackLayer *)malloc(sizeof(StackLayer) * interpreter.stack_layer_capacity);

    interpret_function(&interpreter, apm, apm->main);

    free(interpreter.stack_layer);
}
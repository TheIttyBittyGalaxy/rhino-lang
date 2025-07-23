#include "apm.h"
#include <stdio.h>

// ENUMS //

DEFINE_ENUM(LIST_RHINO_SORTS, RhinoSort, rhino_sort)
DEFINE_ENUM(LIST_EXPR_PRECEDENCE, ExprPrecedence, expr_precedence)
DEFINE_ENUM(LIST_EXPRESSIONS, ExpressionKind, expression_kind)
DEFINE_ENUM(LIST_STATEMENTS, StatementKind, statement_kind)

// LIST TYPE //

DEFINE_LIST_TYPE(Expression, expression)
DEFINE_LIST_TYPE(Statement, statement)
DEFINE_LIST_TYPE(Function, function)
DEFINE_LIST_TYPE(Variable, variable)
DEFINE_LIST_TYPE(EnumType, enum_type)
DEFINE_LIST_TYPE(EnumValue, enum_value)

DEFINE_SLICE_TYPE(Expression, expression)
DEFINE_SLICE_TYPE(Statement, statement)
DEFINE_SLICE_TYPE(Function, function)
DEFINE_SLICE_TYPE(Variable, variable)
DEFINE_SLICE_TYPE(EnumType, enum_type)
DEFINE_SLICE_TYPE(EnumValue, enum_value)

// RHINO TYPE //

const char *rhino_type_string(Program *apm, RhinoType ty)
{
    // TODO: Return name of enum type instead of sort
    return rhino_sort_string(ty.sort);
}

// INIT APM //

void init_program(Program *apm)
{
    init_function_list(&apm->function);
    init_statement_list(&apm->statement);
    init_expression_list(&apm->expression);
    init_variable_list(&apm->variable);
    init_enum_type_list(&apm->enum_type);
    init_enum_value_list(&apm->enum_value);
}

// DUMP PROGRAM //

void dump_apm(Program *apm, const char *source_text)
{
    printf("FUNCTIONS\n");
    for (size_t i = 0; i < apm->function.count; i++)
    {
        Function *funct = get_function(apm->function, i);
        printf("%02d\t%03d %02d\t", i, funct->span.pos, funct->span.len);
        printf_substr(source_text, funct->identity);
        printf("\tbody %02d\n", funct->body);
    }
    printf("\n");

    printf("STATEMENTS\n");
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);
        printf("%02d\t%03d %02d\t%-21s\t", i, stmt->span.pos, stmt->span.len, statement_kind_string(stmt->kind));
        switch (stmt->kind)
        {
        case CODE_BLOCK:
        case SINGLE_BLOCK:
            printf("first %02d\tlast %02d", stmt->statements.first, stmt->statements.first + stmt->statements.count - 1);
            break;

        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
            printf("body %02d\tcondition %02d", stmt->body, stmt->condition);
            break;

        case FOR_LOOP:
            printf("body %02d\titerator %02d\tfirst %02d\tlast %02d", stmt->body, stmt->iterator, stmt->first, stmt->last);
            break;

        case ELSE_SEGMENT:
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
        printf("%02d\t%03d %02d\t%s\t", i, expr->span.pos, expr->span.len, expression_kind_string(expr->kind));
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

        case VARIABLE_REFERENCE:
            printf("variable %02d", expr->variable);
            break;

        case TYPE_REFERENCE:
            printf("\ttype %s", rhino_type_string(apm, expr->type));
            break;

        case FUNCTION_CALL:
            printf("callee %02d", expr->callee);
            break;

        case INDEX_BY_FIELD:
            printf("\tsubject: %02d\tfield: ", expr->subject);
            printf_substr(source_text, expr->field);
            break;
        }
        printf("\n");
    }
    printf("\n");

    printf("VARIABLES\n");
    for (size_t i = 0; i < apm->variable.count; i++)
    {
        Variable *var = get_variable(apm->variable, i);
        printf("%02d\t", i);
        printf_substr(source_text, var->identity);
        printf("\t%s\n", rhino_type_string(apm, var->type));
    }
    printf("\n");

    printf("ENUM TYPES\n");
    for (size_t i = 0; i < apm->enum_type.count; i++)
    {
        EnumType *enum_type = get_enum_type(apm->enum_type, i);
        printf("%02d\t", i);
        printf_substr(source_text, enum_type->identity);
        printf("\tfirst %02d\tlast %02d", enum_type->values.first, enum_type->values.first + enum_type->values.count - 1);
        printf("\n");
    }
    printf("\n");

    printf("ENUM VALUES\n");
    for (size_t i = 0; i < apm->enum_value.count; i++)
    {
        EnumValue *enum_value = get_enum_value(apm->enum_value, i);
        printf("%02d\t", i);
        printf_substr(source_text, enum_value->identity);
        printf("\n");
    }
}

// PRINT APM //

#define PRINT_PARSED
#include "include/print-apm.c"

#define PRINT_ANALYSED
#include "include/print-apm.c"

// ACCESS METHODS //

size_t get_next_statement_in_code_block(Program *apm, Statement *code_block, size_t n)
{
    if (code_block->statements.count == 0)
        return 0;

    Statement *child = get_statement(apm->statement, code_block->statements.first + n);
    n++;

    if (child->kind == CODE_BLOCK || child->kind == SINGLE_BLOCK)
        return n + child->statements.count;
    else if (child->kind == IF_SEGMENT || child->kind == ELSE_IF_SEGMENT || child->kind == ELSE_SEGMENT || child->kind == FOR_LOOP)
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

// TYPE ANALYSIS METHODS //

RhinoType get_expression_type(Program *apm, size_t expr_index)
{
    RhinoType result;
    result.sort = INVALID_SORT;
    result.index = 0;

    Expression *expr = get_expression(apm->expression, expr_index);

    switch (expr->kind)
    {
    // Literals
    case BOOLEAN_LITERAL:
        result.sort = SORT_BOOL;
        break;

    case STRING_LITERAL:
        result.sort = SORT_STR;
        break;

    case INTEGER_LITERAL:
        result.sort = SORT_INT;
        break;

    case FLOAT_LITERAL:
        result.sort = SORT_NUM;
        break;

    case ENUM_VALUE_LITERAL:
        result.sort = SORT_ENUM;
        result.index = get_enum_type_of_enum_value(apm, expr_index);
        break;

    // Variables
    case VARIABLE_REFERENCE:
        return get_variable(apm->variable, expr->variable)->type;

    // Numerical operations
    case BINARY_DIVIDE:
        result.sort = SORT_NUM;
        break;

    case BINARY_MULTIPLY:
    case BINARY_REMAINDER:
    case BINARY_ADD:
    case BINARY_SUBTRACT:
    {
        RhinoType lhs_type = get_expression_type(apm, expr->lhs);
        RhinoType rhs_type = get_expression_type(apm, expr->rhs);
        if (lhs_type.sort == SORT_INT && rhs_type.sort == SORT_INT)
            result.sort = SORT_INT;
        else
            result.sort = SORT_NUM;
        break;
    }

    // Logical operations
    case BINARY_LESS_THAN:
    case BINARY_GREATER_THAN:
    case BINARY_LESS_THAN_EQUAL:
    case BINARY_GREATER_THAN_EQUAL:
    case BINARY_EQUAL:
    case BINARY_NOT_EQUAL:
    case BINARY_LOGICAL_AND:
    case BINARY_LOGICAL_OR:
        result.sort = SORT_BOOL;
        break;
    }

    return result;
}

size_t get_enum_type_of_enum_value(Program *apm, size_t enum_value_index)
{
    size_t last = 0;
    for (size_t i = 0; i < apm->enum_type.count; i++)
    {
        EnumType *enum_type = get_enum_type(apm->enum_type, i);
        last += enum_type->values.count;

        if (enum_value_index < last)
            return i;
    }

    // FIXME: Unreachable, treat this as a fatal error
    return 0;
}

bool is_expression_boolean(Program *apm, size_t expr_index)
{
    return get_expression_type(apm, expr_index).sort == SORT_BOOL;
}

bool are_types_equal(RhinoType a, RhinoType b)
{
    if (a.sort != b.sort)
        return false;

    if (a.sort == SORT_ENUM && a.index != b.index)
        return false;

    return true;
}

bool can_assign_a_to_b(RhinoType a, RhinoType b)
{
    if (are_types_equal(a, b))
        return true;

    if (a.sort == SORT_INT && b.sort == SORT_NUM)
        return true;

    return false;
}

// EXPRESSION PRECEDENCE METHODS //

ExprPrecedence precedence_of(ExpressionKind expr_kind)
{
    switch (expr_kind)
    {
    case INVALID_EXPRESSION:
        return PRECEDENCE_NONE;

    case IDENTITY_LITERAL:
    case INTEGER_LITERAL:
    case FLOAT_LITERAL:
    case BOOLEAN_LITERAL:
    case STRING_LITERAL:
    case VARIABLE_REFERENCE:
        return PRECEDENCE_NONE;

    case FUNCTION_CALL:
        return PRECEDENCE_CALL;

    case INDEX_BY_FIELD:
        return PRECEDENCE_INDEX;

    case BINARY_MULTIPLY:
    case BINARY_DIVIDE:
    case BINARY_REMAINDER:
        return PRECEDENCE_FACTOR;

    case BINARY_ADD:
    case BINARY_SUBTRACT:
        return PRECEDENCE_TERM;

    case BINARY_LESS_THAN:
    case BINARY_GREATER_THAN:
    case BINARY_LESS_THAN_EQUAL:
    case BINARY_GREATER_THAN_EQUAL:
        return PRECEDENCE_COMPARE_RELATIVE;

    case BINARY_EQUAL:
    case BINARY_NOT_EQUAL:
        return PRECEDENCE_COMPARE_EQUAL;

    case BINARY_LOGICAL_AND:
        return PRECEDENCE_LOGICAL_AND;

    case BINARY_LOGICAL_OR:
        return PRECEDENCE_LOGICAL_OR;
    }

    // TODO: Ensure this is unreachable
    return PRECEDENCE_NONE;
}

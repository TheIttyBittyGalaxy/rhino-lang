#include "apm.h"
#include <stdio.h>
#include "../fatal_error.h"

// ENUMS //

DEFINE_ENUM(LIST_RHINO_SORTS, RhinoSort, rhino_sort)
DEFINE_ENUM(LIST_EXPR_PRECEDENCE, ExprPrecedence, expr_precedence)
DEFINE_ENUM(LIST_EXPRESSIONS, ExpressionKind, expression_kind)
DEFINE_ENUM(LIST_STATEMENTS, StatementKind, statement_kind)
DEFINE_ENUM(LIST_SYMBOL_TAG, SymbolTag, symbol_tag)

// LIST TYPE //

DEFINE_LIST_TYPE(Expression, expression)
DEFINE_LIST_TYPE(Statement, statement)
DEFINE_LIST_TYPE(Function, function)
DEFINE_LIST_TYPE(Variable, variable)
DEFINE_LIST_TYPE(EnumType, enum_type)
DEFINE_LIST_TYPE(EnumValue, enum_value)
DEFINE_LIST_TYPE(SymbolTable, symbol_table)

DEFINE_SLICE_TYPE(Expression, expression)
DEFINE_SLICE_TYPE(Statement, statement)
DEFINE_SLICE_TYPE(Function, function)
DEFINE_SLICE_TYPE(Variable, variable)
DEFINE_SLICE_TYPE(EnumType, enum_type)
DEFINE_SLICE_TYPE(EnumValue, enum_value)
DEFINE_SLICE_TYPE(SymbolTable, symbol_table)

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

    init_symbol_table_list(&apm->symbol_table);

    // Symbol tables treat `symbol_table.next = 0` as `symbol_table.next = NULL`. and so the first symbol table has to be empty
    add_symbol_table(&apm->symbol_table);
}

// SYMBOL TABLES //

void init_symbol_table(SymbolTable *symbol_table)
{
    symbol_table->next = 0;
    symbol_table->symbol_count = 0;
}

void append_symbol(Program *apm, size_t table_index, SymbolTag symbol_tag, size_t symbol_index, substr symbol_identity)
{
    SymbolTable *table = get_symbol_table(apm->symbol_table, table_index);
    while (table->symbol_count == SYMBOL_TABLE_SIZE && table->next)
    {
        table_index = table->next;
        table = get_symbol_table(apm->symbol_table, table_index);
    }

    if (table->symbol_count == SYMBOL_TABLE_SIZE)
    {
        size_t next_index = add_symbol_table(&apm->symbol_table);
        SymbolTable *next = get_symbol_table(apm->symbol_table, next_index);

        // We cannot use the `table` pointer here as the table may been reallocated after we called add_symbol_table
        get_symbol_table(apm->symbol_table, table_index)->next = next_index;

        next->next = 0;
        next->symbol_count = 1;
        next->symbol[0].index = symbol_index;
        next->symbol[0].tag = symbol_tag;
        next->symbol[0].identity = symbol_identity;
        return;
    }

    size_t i = table->symbol_count++;
    table->symbol[i].index = symbol_index;
    table->symbol[i].tag = symbol_tag;
    table->symbol[i].identity = symbol_identity;
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
            printf("first %02d\tlast %02d\tsymbol table %02d", stmt->statements.first, stmt->statements.first + stmt->statements.count - 1, stmt->symbol_table);
            break;

        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
            printf("body %02d\tcondition %02d", stmt->body, stmt->condition);
            break;

        case FOR_LOOP:
            printf("body %02d\titerator %02d\titerable %02d", stmt->body, stmt->iterator, stmt->iterable);
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

        case UNARY_POS:
        case UNARY_NEG:
            printf("\toperand: %02d", expr->operand);
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
            printf("\tlhs: %02d\trhs: %02d", expr->lhs, expr->rhs);
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

    printf("SYMBOL TABLES\n");
    for (size_t i = 1; i < apm->symbol_table.count; i++)
    {
        SymbolTable *symbol_table = get_symbol_table(apm->symbol_table, i);
        printf("%02d -> %02d", i, symbol_table->next);
        for (size_t i = 0; i < symbol_table->symbol_count; i++)
        {
            printf("\n");
            Symbol symbol = symbol_table->symbol[i];
            printf("\t%02d\t%s\t%02d\t", i, symbol_tag_string(symbol.tag), symbol.index);
            printf_substr(source_text, symbol.identity);
        }
        printf("\n");
    }
    printf("\n");
}

// PRINT APM //

#define PRINT_PARSED
#include "include/print-apm.c"

#define PRINT_RESOLVED
#include "include/print-apm.c"

// ACCESS METHODS //

size_t get_next_statement_in_block(Program *apm, Statement *code_block, size_t n)
{
    if (code_block->statements.count == 0)
        return 0;

    Statement *child = get_statement(apm->statement, code_block->statements.first + n);
    n++;

    switch (child->kind)
    {
    case DECLARATION_BLOCK:
    case CODE_BLOCK:
    case SINGLE_BLOCK:
        return n + child->statements.count;

    case IF_SEGMENT:
    case ELSE_IF_SEGMENT:
    case ELSE_SEGMENT:
    case BREAK_LOOP:
    case FOR_LOOP:
        return n + get_statement(apm->statement, child->body)->statements.count + 1;

    case FUNCTION_DECLARATION:
    {
        Function *funct = get_function(apm->function, child->function);
        return n + get_statement(apm->statement, funct->body)->statements.count + 1;
    }
    }

    return n;
}

size_t get_first_statement_in_block(Program *apm, Statement *code_block)
{
    return 0;
}

size_t get_last_statement_in_block(Program *apm, Statement *code_block)
{
    if (code_block->statements.count == 0)
        return 0;

    size_t n = 0;
    while (true)
    {
        size_t next = get_next_statement_in_block(apm, code_block, n);
        if (next >= code_block->statements.count)
            return n;
        n = next;
    }
}

// TYPE ANALYSIS METHODS //

RhinoType get_expression_type(Program *apm, size_t expr_index)
{
    RhinoType result;
    Expression *expr = get_expression(apm->expression, expr_index);

    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        result.sort = ERROR_SORT;
        break;

    // Literals
    case IDENTITY_LITERAL:
        result.sort = ERROR_SORT;
        break;

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
    {
        Expression *enum_value_literal = get_expression(apm->expression, expr_index);
        result.sort = SORT_ENUM;
        result.index = get_enum_type_of_enum_value(apm, enum_value_literal->enum_value);
        break;
    }

    // Variables
    case VARIABLE_REFERENCE:
        return get_variable(apm->variable, expr->variable)->type;

    // Function call
    case FUNCTION_CALL:
    {
        Expression *callee = get_expression(apm->expression, expr->callee);

        if (callee->kind != FUNCTION_REFERENCE)
        {
            result.sort = ERROR_SORT;
            break;
        }

        Function *funct = get_function(apm->function, callee->function);
        return funct->return_type;
    }

    // Index by field
    case INDEX_BY_FIELD:
    {
        // TODO: If the subject can be indexed then return the type of the appropriate field
        result.sort = ERROR_SORT;
        break;
    }

    // Numerical operations
    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        return get_expression_type(apm, expr->operand);

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

    default:
        fatal_error("Could not determine type of %s expression.", expression_kind_string(expr->kind));
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

    fatal_error("Could not determine index of enum value %d.", enum_value_index);
    return 0;
}

bool are_types_equal(RhinoType a, RhinoType b)
{
    if (a.sort != b.sort)
        return false;

    if (a.sort == SORT_ENUM && a.index != b.index)
        return false;

    return true;
}

bool allow_assign_a_to_b(RhinoType a, RhinoType b)
{
    if (a.sort == ERROR_SORT || b.sort == ERROR_SORT)
        return true;

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

    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        return PRECEDENCE_CALL_OR_INCREMENT;

    case FUNCTION_CALL:
        return PRECEDENCE_CALL_OR_INCREMENT;

    case INDEX_BY_FIELD:
        return PRECEDENCE_INDEX;

    case RANGE_LITERAL:
        return PRECEDENCE_RANGE;

    case UNARY_POS:
    case UNARY_NEG:
        return PRECEDENCE_UNARY;

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

    default:
        fatal_error("Could not determine precedence of %s expression.", expr_kind);
        break;
    }

    return PRECEDENCE_NONE;
}

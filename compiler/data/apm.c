#include "apm.h"
#include <stdio.h>
#include "../fatal_error.h"

// ENUMS //

DEFINE_ENUM(LIST_RHINO_SORTS, RhinoSort, rhino_sort)
DEFINE_ENUM(LIST_EXPR_PRECEDENCE, ExprPrecedence, expr_precedence)
DEFINE_ENUM(LIST_EXPRESSIONS, ExpressionKind, expression_kind)
DEFINE_ENUM(LIST_STATEMENTS, StatementKind, statement_kind)
DEFINE_ENUM(LIST_SYMBOL_TAG, SymbolTag, symbol_tag)

// ALLOCATORS //

DEFINE_LIST_ALLOCATOR(EnumValue, enum_value)
DEFINE_LIST_ALLOCATOR(EnumType, enum_type)

DEFINE_LIST_ALLOCATOR(Property, property)
DEFINE_LIST_ALLOCATOR(StructType, struct_type)

DEFINE_LIST_ALLOCATOR(Variable, variable)

DEFINE_LIST_ALLOCATOR(Expression, expression)

DEFINE_LIST_ALLOCATOR(Statement, statement)

DEFINE_LIST_ALLOCATOR(Block, block)
DEFINE_LIST_ALLOCATOR(SymbolTable, symbol_table)

DEFINE_LIST_ALLOCATOR(Function, function)
DEFINE_LIST_ALLOCATOR(Parameter, parameter)
DEFINE_LIST_ALLOCATOR(Argument, argument)

// RHINO TYPE //

const char *rhino_type_string(Program *apm, RhinoType ty)
{
    // TODO: Return name of enum type instead of sort
    return rhino_sort_string(ty.sort);
}

// INIT APM //

void init_program(Program *apm, Allocator *allocator)
{
    init_allocator(&apm->statement_lists, allocator, 1024);
    init_allocator(&apm->enum_value_lists, allocator, 1024);
    init_allocator(&apm->property_lists, allocator, 1024);
    init_allocator(&apm->parameter_lists, allocator, 1024);
    init_allocator(&apm->arguments_lists, allocator, 1024);

    init_allocator(&apm->symbol_table, allocator, 1024);

    init_expression_list_allocator(&apm->expression, allocator, 1024);
    init_function_list_allocator(&apm->function, allocator, 1024);
    init_variable_list_allocator(&apm->variable, allocator, 1024);
    init_enum_type_list_allocator(&apm->enum_type, allocator, 1024);
    init_struct_type_list_allocator(&apm->struct_type, allocator, 1024);
    init_block_list_allocator(&apm->block, allocator, 1024);
}

// SYMBOL TABLES //

SymbolTable *allocate_symbol_table(Allocator *allocator, SymbolTable *parent)
{
    SymbolTable *table = (SymbolTable *)allocate(allocator, sizeof(SymbolTable), alignof(SymbolTable));
    table->next = parent;
    table->symbol_count = 0;
    return table;
}

void append_symbol(Program *apm, SymbolTable *table, SymbolTag symbol_tag, SymbolPointer to, substr symbol_identity)
{
    while (table->symbol_count == SYMBOL_TABLE_SIZE && table->next)
        table = table->next;

    if (table->symbol_count == SYMBOL_TABLE_SIZE)
    {
        SymbolTable *next = allocate_symbol_table(&apm->symbol_table, table->next);
        table->next = next;

        next->next = 0;
        next->symbol_count = 1;
        next->symbol[0].to = to;
        next->symbol[0].tag = symbol_tag;
        next->symbol[0].identity = symbol_identity;
        return;
    }

    size_t i = table->symbol_count++;
    table->symbol[i].to = to;
    table->symbol[i].tag = symbol_tag;
    table->symbol[i].identity = symbol_identity;
}

// DUMP PROGRAM //

void dump_apm(Program *apm, const char *source_text)
{
    printf("FUNCTIONS\n");
    Function *funct;
    size_t funct_i = 0;
    for (
        FunctionIterator it = function_iterator(get_function_list(apm->function));
        funct = next_function_iterator(&it); funct_i++)
    {
        printf("%02d\t%03d %02d\t", funct_i, funct->span.pos, funct->span.len);
        printf_substr(source_text, funct->identity);
        printf("\tbody %02d\n", funct->body);
    }
    printf("\n");

    /*
    printf("STATEMENTS\n");
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);
        printf("%02d\t%03d %02d\t%-21s\t", i, stmt->span.pos, stmt->span.len, statement_kind_string(stmt->kind));
        switch (stmt->kind)
        {
        case DECLARATION_BLOCK:
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
    */

    printf("EXPRESSIONS\n");
    Expression *expr;
    size_t expr_i = 0;
    for (
        ExpressionIterator it = expression_iterator(get_expression_list(apm->expression));
        expr = next_expression_iterator(&it); expr_i++)
    {
        printf("%02d\t%03d %02d\t%s\t", expr_i, expr->span.pos, expr->span.len, expression_kind_string(expr->kind));
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
    Variable *var;
    size_t var_i = 0;
    for (
        VariableIterator it = variable_iterator(get_variable_list(apm->variable));
        var = next_variable_iterator(&it); var_i++)
    {
        printf("%02d\t", var_i);
        printf_substr(source_text, var->identity);
        printf("\t%s\n", rhino_type_string(apm, var->type));
    }
    printf("\n");

    printf("ENUM TYPES\n");
    EnumType *enum_type;
    size_t enum_type_i = 0;
    for (
        EnumTypeIterator it = enum_type_iterator(get_enum_type_list(apm->enum_type));
        enum_type = next_enum_type_iterator(&it); enum_type_i++)
    {
        printf("%02d\t", enum_type_i);
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
    printf("\n");

    printf("STRUCT TYPES\n");
    StructType *struct_type;
    size_t struct_type_i = 0;
    for (
        StructTypeIterator it = struct_type_iterator(get_struct_type_list(apm->struct_type));
        struct_type = next_struct_type_iterator(&it); struct_type_i++)
    {
        printf("%02d\t", struct_type_i);
        printf_substr(source_text, struct_type->identity);
        printf("\tfirst %02d\tlast %02d", struct_type->properties.first, struct_type->properties.first + struct_type->properties.count - 1);
        printf("\n");
    }
    printf("\n");

    /*
    printf("PROPERTIES\n");
    for (size_t i = 0; i < apm->property.count; i++)
    {
        Property *property = get_property(apm->property, i);
        printf("%02d\t", i);
        printf_substr(source_text, property->identity);
        printf("\ttype_expr: %02d", property->type_expression);
        printf("\n");
    }
    printf("\n");
    */

    /*
    printf("SYMBOL TABLES\n");
    for (size_t i = 1; i < apm->symbol_table.count; i++)
    {
        SymbolTable *symbol_table = get_symbol_table(apm->symbol_table, i);
        printf("%02d -> %02d", i, symbol_table->next);
        for (size_t i = 0; i < symbol_table->symbol_count; i++)
        {
            printf("\n");
            Symbol symbol = symbol_table->symbol[i];
            printf("\t%02d\t%s\t%p\t", i, symbol_tag_string(symbol.tag), symbol.to.ptr);
            printf_substr(source_text, symbol.identity);
        }
        printf("\n");
    }
    printf("\n");
    */
}

// PRINT APM //

#define PRINT_PARSED
#include "include/print-apm.c"

#define PRINT_RESOLVED
#include "include/print-apm.c"

// TYPE ANALYSIS METHODS //

RhinoType get_expression_type(Program *apm, const char *source_text, Expression *expr)
{
    RhinoType result;

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
        result.sort = SORT_ENUM;
        result.enum_type = expr->enum_value->type_of_enum_value;
        break;
    }

    // Variables and parameters
    case VARIABLE_REFERENCE:
        return expr->variable->type;

    case PARAMETER_REFERENCE:
        return expr->parameter->type;

    // Function call
    case FUNCTION_CALL:
    {
        Expression *callee = expr->callee;

        if (callee->kind != FUNCTION_REFERENCE)
        {
            result.sort = ERROR_SORT;
            break;
        }

        Function *funct = callee->function;
        return funct->return_type;
    }

    // Index by field
    case INDEX_BY_FIELD:
    {
        RhinoType subject_type = get_expression_type(apm, source_text, expr->subject);
        if (subject_type.sort == SORT_STRUCT)
        {
            StructType *struct_type = subject_type.struct_type;
            Property *property;
            PropertyIterator it = property_iterator(struct_type->properties);
            while (property = next_property_iterator(&it))
            {
                if (substr_match(source_text, property->identity, expr->field))
                    return property->type;
            }
        }

        result.sort = ERROR_SORT;
        break;
    }

    // Numerical operations
    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        return get_expression_type(apm, source_text, expr->operand);

    case BINARY_DIVIDE:
        result.sort = SORT_NUM;
        break;

    case BINARY_MULTIPLY:
    case BINARY_REMAINDER:
    case BINARY_ADD:
    case BINARY_SUBTRACT:
    {
        RhinoType lhs_type = get_expression_type(apm, source_text, expr->lhs);
        RhinoType rhs_type = get_expression_type(apm, source_text, expr->rhs);
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

bool are_types_equal(RhinoType a, RhinoType b)
{
    if (a.sort != b.sort)
        return false;

    if (a.sort == SORT_ENUM && a.enum_type != b.enum_type)
        return false;

    if (a.sort == SORT_STRUCT && a.enum_type != b.enum_type)
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

#include "apm.h"

// ENUMS //

DEFINE_ENUM(LIST_RHINO_TYPE_TAG, RhinoTypeTag, rhino_type_tag)
DEFINE_ENUM(LIST_EXPR_PRECEDENCE, ExprPrecedence, expr_precedence)
DEFINE_ENUM(LIST_EXPRESSIONS, ExpressionKind, expression_kind)
DEFINE_ENUM(LIST_STATEMENTS, StatementKind, statement_kind)
DEFINE_ENUM(LIST_SYMBOL_TAG, SymbolTag, symbol_tag)

// Create strongly-typed lists from allocators

#define DEFINE_LIST_OF(T, snake_case)                        \
    T##List create_##snake_case##_list(Allocator *allocator) \
    {                                                        \
        return (T##List){                                    \
            .bucket = allocator->first,                      \
            .count = count_chunks_of(allocator->first, T),   \
        };                                                   \
    }                                                        \
                                                             \
    Iterator create_iterator(T##List *list)                  \
    {                                                        \
        return create_iterator(list->bucket);                \
    }                                                        \
                                                             \
    T *get_##snake_case(T##List *list, size_t i)             \
    {                                                        \
        return get_chunk_of(list->bucket, i, T);             \
    }

DEFINE_LIST_OF(Argument, argument)
DEFINE_LIST_OF(EnumValue, enum_value)
DEFINE_LIST_OF(Expression, expression)
DEFINE_LIST_OF(Parameter, parameter)
DEFINE_LIST_OF(Property, property)
DEFINE_LIST_OF(Statement, statement)

// RHINO TYPE //

// FIXME: Indicate if the type is noneable
const char *rhino_type_string(Program *apm, RhinoType ty)
{
    switch (ty.tag)
    {

    case RHINO_INVALID_TYPE_TAG:
        return "INVALID_TYPE";

    case RHINO_UNINITIALISED_TYPE_TAG:
        return "UNINITIALISED_TYPE";

    case RHINO_ERROR_TYPE:
        return "ERROR_TYPE";

    case RHINO_NATIVE_TYPE:
        return ty.native_type->name;

    case RHINO_ENUM_TYPE:
        // FIXME: Return the name of the struct, rather than the tag name
        return "ENUM_TYPE";

    case RHINO_STRUCT_TYPE:
        // FIXME: Return the name of the struct, rather than the tag name
        return "STRUCT_TYPE";

    default:
        fatal_error("Could not stringify %s Rhino type.", rhino_type_tag_string(ty.tag));
        break;
    }

    unreachable;
}

// SYMBOL TABLES //

SymbolTable *allocate_symbol_table(Allocator *allocator, SymbolTable *parent)
{
    SymbolTable *table = (SymbolTable *)allocate(allocator, SymbolTable);
    table->next = parent;
    table->symbol_count = 0;
    return table;
}

void declare_symbol(Allocator *allocator, SymbolTable *table, SymbolTag tag, void *ptr, substr identity)
{
    while (table->symbol_count == SYMBOL_TABLE_SIZE && table->next)
        table = table->next;

    if (table->symbol_count == SYMBOL_TABLE_SIZE)
    {
        SymbolTable *next = allocate_symbol_table(allocator, table->next);
        table->next = next;

        next->next = 0;
        next->symbol_count = 1;
        next->symbol[0].ptr = ptr;
        next->symbol[0].tag = tag;
        next->symbol[0].identity = identity;
        return;
    }

    size_t i = table->symbol_count++;
    table->symbol[i].ptr = ptr;
    table->symbol[i].tag = tag;
    table->symbol[i].identity = identity;
}

// PRINT APM //

#define PRINT_PARSED
#include "../include/print-apm.c"

#define PRINT_RESOLVED
#include "../include/print-apm.c"

// STATEMENT METHODS //

bool is_declaration(Statement *stmt)
{
    return stmt->kind == FUNCTION_DECLARATION ||
           stmt->kind == STRUCT_TYPE_DECLARATION ||
           stmt->kind == ENUM_TYPE_DECLARATION ||
           stmt->kind == VARIABLE_DECLARATION;
}

// TYPE ANALYSIS METHODS //

RhinoType get_expression_type(Program *apm, const char *source_text, Expression *expr)
{
    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        return ERROR_TYPE;

    // Literals
    case IDENTITY_LITERAL:
        return ERROR_TYPE;

    case NONE_LITERAL:
        return NATIVE_NONE;

    case BOOLEAN_LITERAL:
        return NATIVE_BOOL;

    case INTEGER_LITERAL:
        return NATIVE_INT;

    case FLOAT_LITERAL:
        return NATIVE_NUM;

    case STRING_LITERAL:
        return NATIVE_STR;

    case ENUM_VALUE_LITERAL:
        return ENUM_TYPE(expr->enum_value->type_of_enum_value);

    // Variables and parameters
    case VARIABLE_REFERENCE:
        return expr->variable->type;

    case PARAMETER_REFERENCE:
        return expr->parameter->type;

    // Function call
    case FUNCTION_CALL:
        if (expr->callee->kind == FUNCTION_REFERENCE)
            return expr->callee->function->return_type;

        return ERROR_TYPE;

    // Index by field
    case INDEX_BY_FIELD:
    {
        RhinoType subject_type = get_expression_type(apm, source_text, expr->subject);

        if (subject_type.tag == RHINO_STRUCT_TYPE)
        {
            Property *property;
            Iterator it = create_iterator(subject_type.struct_type->properties.bucket);
            while (property = advance_iterator_of(&it, Property))
            {
                if (substr_match(source_text, property->identity, expr->field))
                    return property->type;
            }
        }

        return ERROR_TYPE;
    }

    // Numerical operations
    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        return get_expression_type(apm, source_text, expr->operand);

    case UNARY_NOT:
        return NATIVE_BOOL;

    case BINARY_DIVIDE:
        return NATIVE_NUM;

    case BINARY_MULTIPLY:
    case BINARY_REMAINDER:
    case BINARY_ADD:
    case BINARY_SUBTRACT:
    {
        RhinoType lhs_type = get_expression_type(apm, source_text, expr->lhs);
        RhinoType rhs_type = get_expression_type(apm, source_text, expr->rhs);
        if (IS_INT_TYPE(lhs_type) && IS_INT_TYPE(rhs_type))
            return NATIVE_INT;

        return NATIVE_NUM;
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
        return NATIVE_BOOL;

    default:
        fatal_error("Could not determine type of %s expression.", expression_kind_string(expr->kind));
        break;
    }

    unreachable;
}

// NOTE: The behaviour of this function has not been consider for RhinoTypes
//       RHINO_INVALID_TYPE_TAG / RHINO_UNINITIALISED_TYPE_TAG / RHINO_ERROR_TYPE
bool are_types_equal(RhinoType a, RhinoType b)
{
    if (a.tag != b.tag)
        return false;

    if (a.is_noneable != b.is_noneable)
        return false;

    if (a.tag == RHINO_NATIVE_TYPE && a.native_type != b.native_type)
        return false;

    if (a.tag == RHINO_ENUM_TYPE && a.enum_type != b.enum_type)
        return false;

    if (a.tag == RHINO_STRUCT_TYPE && a.enum_type != b.enum_type)
        return false;

    return true;
}

bool is_native_type(RhinoType ty, NativeType *native_type)
{
    return ty.tag == RHINO_NATIVE_TYPE && !ty.is_noneable && ty.native_type == native_type;
}

bool allow_assign_a_to_b(Program *apm, RhinoType a, RhinoType b)
{
    if (!IS_VALID_TYPE(a) || !IS_VALID_TYPE(b))
        return true;

    if (are_types_equal(a, b))
        return true;

    if (IS_INT_TYPE(a) && IS_NUM_TYPE(b))
        return true;

    if (a.is_noneable && b.is_noneable)
        return true;

    if (!a.is_noneable && b.is_noneable)
    {
        RhinoType b_ = b;
        b_.is_noneable = false;
        if (allow_assign_a_to_b(apm, a, b_))
            return true;
    }

    return false;
}

// EXPRESSION PRECEDENCE METHODS //

ExprPrecedence precedence_of(ExpressionKind expr_kind)
{
    switch (expr_kind)
    {
    case INVALID_EXPRESSION:
        return PRECEDENCE_NONE;

    case NONE_LITERAL:
    case IDENTITY_LITERAL:
    case INTEGER_LITERAL:
    case FLOAT_LITERAL:
    case BOOLEAN_LITERAL:
    case STRING_LITERAL:
    case ENUM_VALUE_LITERAL:
        return PRECEDENCE_IMMEDIATE_VALUE;

    case VARIABLE_REFERENCE:
    case PARAMETER_REFERENCE:
        return PRECEDENCE_IMMEDIATE_VALUE;

    case NONEABLE_EXPRESSION:
        return PRECEDENCE_MODIFIER;

    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        return PRECEDENCE_MODIFIER;

    case FUNCTION_CALL:
        return PRECEDENCE_MODIFIER;

    case INDEX_BY_FIELD:
        return PRECEDENCE_INDEX;

    case RANGE_LITERAL:
        return PRECEDENCE_RANGE;

    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_NOT:
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
        fatal_error("Could not determine precedence of %s expression.", expression_kind_string(expr_kind));
        break;
    }

    unreachable;
}

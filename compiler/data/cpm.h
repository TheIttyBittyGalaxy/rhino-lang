#ifndef CPM_H
#define CPM_H

#include "macro.h"
#include "../memory.h"
#include "substr.h"
#include <stdbool.h>

// Forward Declarations

typedef struct CType CType;

typedef struct CEnumValue CEnumValue;
typedef struct CEnumType CEnumType;
DECLARE_LIST_ALLOCATOR(CEnumValue, c_enum_value)
DECLARE_LIST_ALLOCATOR(CEnumType, c_enum_type)

typedef struct CVariable CVariable;
DECLARE_LIST_ALLOCATOR(CVariable, c_variable)

typedef struct CVarDeclaration CVarDeclaration;
DECLARE_LIST_ALLOCATOR(CVarDeclaration, c_var_declaration);

typedef struct CExpression CExpression;
DECLARE_LIST_ALLOCATOR(CExpression, c_expression);

typedef struct CStatement CStatement;
DECLARE_LIST_ALLOCATOR(CStatement, c_statement);

typedef struct CBlock CBlock;
DECLARE_LIST_ALLOCATOR(CBlock, c_block)

typedef struct CFunction CFunction;
typedef struct CParameter CParameter;
typedef struct CArgument CArgument;
DECLARE_LIST_ALLOCATOR(CFunction, c_function)
DECLARE_LIST_ALLOCATOR(CParameter, c_parameter)
DECLARE_LIST_ALLOCATOR(CArgument, c_argument)

typedef struct CProgram CProgram;

// Identity string

typedef struct
{
    bool from_source;
    union
    {
        const char *str;
        substr sub;
    };
} CIdentity;

// Types

#define LIST_C_TYPE_TAG(MACRO) \
    MACRO(C_VOID)              \
    MACRO(C_BOOL)              \
    MACRO(C_INT)               \
    MACRO(C_FLOAT)             \
    MACRO(C_STRING)            \
    MACRO(C_ENUM_TYPE)         \
    MACRO(C_STRUCT_TYPE)

DECLARE_ENUM(LIST_C_TYPE_TAG, CTypeTag, c_type_tag)

struct CType
{
    CTypeTag tag;
    bool is_const;
    union
    {
        void *ptr;
        CEnumType *enum_type; // C_ENUM_TYPE
        // CStructType *struct_type; // C_STRUCT_TYPE
    };
};

// Enum

struct CEnumValue
{
    substr identity;
};

struct CEnumType
{
    substr identity;
    CFunction *to_string;
    size_t first;
    size_t count;
};

// Variable

struct CVariable
{
    substr identity;
    CType type;
};

// Variable Declaration

struct CVarDeclaration
{
    CVariable *variable;
    CExpression *initial_value;
};

// Expressions

#define LIST_C_EXPRESSIONS(MACRO)      \
    MACRO(INVALID_C_EXPRESSION)        \
                                       \
    MACRO(C_BOOLEAN_LITERAL)           \
    MACRO(C_INTEGER_LITERAL)           \
    MACRO(C_FLOAT_LITERAL)             \
    MACRO(C_STRING_LITERAL)            \
    MACRO(C_SOURCE_STRING_LITERAL)     \
                                       \
    MACRO(C_VARIABLE_REFERENCE)        \
    MACRO(C_FUNCTION_REFERENCE)        \
    MACRO(C_TYPE_REFERENCE)            \
                                       \
    MACRO(C_FUNCTION_CALL)             \
                                       \
    MACRO(C_UNARY_POS)                 \
    MACRO(C_UNARY_NEG)                 \
    MACRO(C_UNARY_NOT)                 \
    MACRO(C_UNARY_PREFIX_INCREMENT)    \
    MACRO(C_UNARY_PREFIX_DECREMENT)    \
    MACRO(C_UNARY_POSTFIX_INCREMENT)   \
    MACRO(C_UNARY_POSTFIX_DECREMENT)   \
                                       \
    MACRO(C_TYPE_CAST)                 \
    MACRO(C_ASSIGNMENT)                \
    MACRO(C_BINARY_MULTIPLY)           \
    MACRO(C_BINARY_REMAINDER)          \
    MACRO(C_BINARY_DIVIDE)             \
    MACRO(C_BINARY_ADD)                \
    MACRO(C_BINARY_SUBTRACT)           \
    MACRO(C_BINARY_LESS_THAN)          \
    MACRO(C_BINARY_GREATER_THAN)       \
    MACRO(C_BINARY_LESS_THAN_EQUAL)    \
    MACRO(C_BINARY_GREATER_THAN_EQUAL) \
    MACRO(C_BINARY_EQUAL)              \
    MACRO(C_BINARY_NOT_EQUAL)          \
    MACRO(C_BINARY_LOGICAL_AND)        \
    MACRO(C_BINARY_LOGICAL_OR)         \
                                       \
    MACRO(C_TERNARY_CONDITIONAL)

DECLARE_ENUM(LIST_C_EXPRESSIONS, CExpressionKind, c_expression_kind)

struct CExpression
{
    CExpressionKind kind;

    union
    {
        // FIXME: Is it strictly necessary for the CPM to allocate it's own
        //        literal nodes, or can it use reuse the APM?
        struct // BOOLEAN_LITERAL
        {
            bool bool_value;
        };
        struct // INTEGER_LITERAL
        {
            int integer_value;
        };
        struct // FLOAT_LITERAL
        {
            double float_value;
        };
        struct // STRING_LITERAL
        {
            const char *string_value;
        };
        struct // C_SOURCE_STRING_LITERAL
        {
            substr source_string;
        };
        struct // VARIABLE_REFERENCE
        {
            CVariable *variable;
        };

        // FIXME: I'm not sure if this is really needed, or if FUNCTION_CALL.callee can just be a pointer to the CFunction?
        struct // FUNCTION_REFERENCE
        {
            CFunction *function;
        };

        struct // TYPE_REFERENCE
        {
            CType type;
        };
        struct // FUNCTION_CALL
        {
            CExpression *callee;
            CArgumentList arguments;
        };
        struct // UNARY_*
        {
            CExpression *operand;
        };
        struct // BINARY_* / TYPE_CAST
        {
            CExpression *lhs;
            CExpression *rhs;
        };
        struct // TERNARY_CONDITIONAL
        {
            CExpression *condition;
            CExpression *when_true;
            CExpression *when_false;
        };
    };
};

// Statements

#define LIST_C_STATEMENTS(MACRO)  \
    MACRO(INVALID_C_STATEMENT)    \
                                  \
    MACRO(C_VARIABLE_DECLARATION) \
    MACRO(C_EXPRESSION_STMT)      \
                                  \
    MACRO(C_IF_SEGMENT)           \
    MACRO(C_ELSE_IF_SEGMENT)      \
    MACRO(C_ELSE_SEGMENT)         \
                                  \
    MACRO(C_BREAK_LOOP)           \
    MACRO(C_FOR_LOOP)             \
    MACRO(C_WHILE_LOOP)

DECLARE_ENUM(LIST_C_STATEMENTS, CStatementKind, c_statement_kind)

struct CStatement
{
    CStatementKind kind;

    union
    {
        struct // VARIABLE_DECLARATION
        {
            CVarDeclaration var_declaration;
        };
        struct // IF_SEGMENT / ELSE_IF_SEGMENT / ELSE_SEGMENT / WHILE_LOOP
        {
            CBlock *body;
            CExpression *condition;
        };
        struct // BREAK_LOOP
        {
            CBlock *__loop_body; // NOTE: KEEP SYNCED WITH IF_SEGMENT body
        };
        struct // FOR_LOOP
        {
            CBlock *__for_loop_body;           // NOTE: KEEP SYNCED WITH IF_SEGMENT body
            CExpression *__for_loop_condition; // NOTE: KEEP SYNCED WITH IF_SEGMENT condition
            CVarDeclaration init;
            CExpression *advance;
        };
        struct // EXPRESSION_STMT
        {
            CExpression *expr;
        };
    };
};

// Block

struct CBlock
{
    CStatementList statements;
};

// Function

struct CFunction
{
    CIdentity identity;
    CBlock *body;
    CParameterList parameters;
    CType return_type;
};

// Parameter

struct CParameter
{
    substr identity;
};

// Argument

struct CArgument
{
    CExpression *expr;
};

// Program

struct CProgram
{
    Allocator statement_lists;
    Allocator parameter_lists;
    Allocator argument_lists;

    CEnumTypeListAllocator enum_type;
    CEnumValueListAllocator enum_value;
    CVariableListAllocator variable;
    CExpressionListAllocator expression;
    CStatementListAllocator statement;
    CBlockListAllocator block;
    CFunctionListAllocator function;
    CFunctionListAllocator native_function;

    CFunction *printf_function;
    CFunction *enum_to_string_function;
};

void init_c_program(CProgram *cpm, Allocator *allocator);

#endif
#ifndef APM_H
#define APM_H

#include "../core/core.h"

// Forward Declarations

typedef struct RhinoType RhinoType;
typedef struct NativeType NativeType;

typedef struct EnumValue EnumValue;
typedef struct EnumType EnumType;

typedef struct Property Property;
typedef struct StructType StructType;

typedef struct Variable Variable;

typedef struct Expression Expression;

typedef struct Statement Statement;

typedef struct Block Block;
typedef struct SymbolTable SymbolTable;

typedef struct Function Function;
typedef struct Parameter Parameter;
typedef struct Argument Argument;

typedef struct Program Program;

// Create strongly-typed lists from allocators

#define DECLARE_LIST_OF(T, snake_case)                        \
    typedef struct                                            \
    {                                                         \
        Bucket *bucket;                                       \
        size_t count;                                         \
    } T##List;                                                \
                                                              \
    T##List create_##snake_case##_list(Allocator *allocator); \
    Iterator create_iterator(T##List *list);                  \
    T *get_##snake_case(T##List *list, size_t i);

DECLARE_LIST_OF(Argument, argument)
DECLARE_LIST_OF(EnumValue, enum_value)
DECLARE_LIST_OF(Expression, expression)
DECLARE_LIST_OF(Parameter, parameter)
DECLARE_LIST_OF(Property, property)
DECLARE_LIST_OF(Statement, statement)

// Types
#define LIST_RHINO_TYPE_TAG(MACRO)      \
    MACRO(RHINO_INVALID_TYPE_TAG)       \
    MACRO(RHINO_UNINITIALISED_TYPE_TAG) \
                                        \
    /* The type cannot be determined    \
       due to user error. */            \
    MACRO(RHINO_ERROR_TYPE)             \
                                        \
    MACRO(RHINO_NATIVE_TYPE)            \
    MACRO(RHINO_ENUM_TYPE)              \
    MACRO(RHINO_STRUCT_TYPE)

DECLARE_ENUM(LIST_RHINO_TYPE_TAG, RhinoTypeTag, rhino_type_tag)

struct RhinoType
{
    RhinoTypeTag tag;
    bool is_noneable;
    union
    {
        void *ptr;
        NativeType *native_type; // RHINO_NATIVE_TYPE
        EnumType *enum_type;     // RHINO_ENUM_TYPE
        StructType *struct_type; // RHINO_STRUCT_TYPE
    };
};

// RHINO_INVALID_TYPE_TAG / RHINO_UNINITIALISED_TYPE_TAG / RHINO_ERROR_TYPE
#define ERROR_TYPE ((RhinoType){RHINO_ERROR_TYPE, 0})

#define IS_VALID_TYPE(ty) !(ty.tag == RHINO_INVALID_TYPE_TAG ||       \
                            ty.tag == RHINO_UNINITIALISED_TYPE_TAG || \
                            ty.tag == RHINO_ERROR_TYPE)

// RHINO_NATIVE_TYPE
#define NATIVE_NONE ((RhinoType){RHINO_NATIVE_TYPE, true, &apm->none_type})
#define NATIVE_BOOL ((RhinoType){RHINO_NATIVE_TYPE, false, &apm->bool_type})
#define NATIVE_INT ((RhinoType){RHINO_NATIVE_TYPE, false, &apm->int_type})
#define NATIVE_NUM ((RhinoType){RHINO_NATIVE_TYPE, false, &apm->num_type})
#define NATIVE_STR ((RhinoType){RHINO_NATIVE_TYPE, false, &apm->str_type})

// RHINO_ENUM_TYPE
#define ENUM_TYPE(enum_type) ((RhinoType){RHINO_ENUM_TYPE, false, enum_type})

// RHINO_STRUCT_TYPE
#define STRUCT_TYPE(struct_type) ((RhinoType){RHINO_STRUCT_TYPE, false, struct_type})

// Native Type

struct NativeType
{
    const char *name;
};

// Enum value
struct EnumValue
{
    substr span;
    substr identity;
    EnumType *type_of_enum_value; // FIXME: I would like to not have this if possible
};

// Enum type
struct EnumType
{
    substr span;
    substr identity;
    EnumValueList values;
};

// Property
struct Property
{
    substr span;
    substr identity;
    Expression *type_expression;
    RhinoType type;
};

// Struct type
struct StructType
{
    substr span;
    substr identity;
    PropertyList properties;
    Block *body;
};

// Variable
struct Variable
{
    substr identity;
    RhinoType type;
    size_t order; // FIXME: Is there a better name for this?
};

// Symbol table
#define LIST_SYMBOL_TAG(MACRO) \
    MACRO(INVALID_SYMBOL)      \
    MACRO(VARIABLE_SYMBOL)     \
    MACRO(FUNCTION_SYMBOL)     \
    MACRO(PARAMETER_SYMBOL)    \
    MACRO(ENUM_TYPE_SYMBOL)    \
    MACRO(STRUCT_TYPE_SYMBOL)

DECLARE_ENUM(LIST_SYMBOL_TAG, SymbolTag, symbol_tag)

typedef struct
{
    SymbolTag tag;
    union
    {
        void *ptr;
        Function *function;
        Variable *variable;
        Parameter *parameter;
        EnumType *enum_type;
        StructType *struct_type;
    };
    substr identity;
} Symbol;

#define SYMBOL_TABLE_SIZE 16

struct SymbolTable
{
    size_t symbol_count;
    SymbolTable *next;
    Symbol symbol[SYMBOL_TABLE_SIZE];
};

SymbolTable *allocate_symbol_table(Allocator *allocator, SymbolTable *parent);
void declare_symbol(Allocator *allocator, SymbolTable *table, SymbolTag tag, void *ptr, substr identity);

// Expression Precedence
// Ordered from "happens last" to "happens first"
#define LIST_EXPR_PRECEDENCE(MACRO)    \
    MACRO(PRECEDENCE_NONE)             \
    MACRO(PRECEDENCE_LOGICAL_OR)       \
    MACRO(PRECEDENCE_LOGICAL_AND)      \
    MACRO(PRECEDENCE_COMPARE_EQUAL)    \
    MACRO(PRECEDENCE_COMPARE_RELATIVE) \
    MACRO(PRECEDENCE_TERM)             \
    MACRO(PRECEDENCE_FACTOR)           \
    MACRO(PRECEDENCE_RANGE)            \
    MACRO(PRECEDENCE_UNARY)            \
    MACRO(PRECEDENCE_INDEX)            \
    MACRO(PRECEDENCE_MODIFIER)         \
    MACRO(PRECEDENCE_IMMEDIATE_VALUE)

DECLARE_ENUM(LIST_EXPR_PRECEDENCE, ExprPrecedence, expr_precedence)

// Expression
#define LIST_EXPRESSIONS(MACRO)      \
    MACRO(INVALID_EXPRESSION)        \
                                     \
    MACRO(IDENTITY_LITERAL)          \
    MACRO(NONE_LITERAL)              \
    MACRO(BOOLEAN_LITERAL)           \
    MACRO(INTEGER_LITERAL)           \
    MACRO(FLOAT_LITERAL)             \
    MACRO(STRING_LITERAL)            \
    MACRO(ENUM_VALUE_LITERAL)        \
                                     \
    MACRO(VARIABLE_REFERENCE)        \
    MACRO(FUNCTION_REFERENCE)        \
    MACRO(PARAMETER_REFERENCE)       \
    MACRO(TYPE_REFERENCE)            \
                                     \
    MACRO(FUNCTION_CALL)             \
                                     \
    MACRO(INDEX_BY_FIELD)            \
                                     \
    MACRO(RANGE_LITERAL)             \
                                     \
    MACRO(NONEABLE_EXPRESSION)       \
                                     \
    MACRO(UNARY_POS)                 \
    MACRO(UNARY_NEG)                 \
    MACRO(UNARY_NOT)                 \
    MACRO(UNARY_INCREMENT)           \
    MACRO(UNARY_DECREMENT)           \
                                     \
    MACRO(BINARY_MULTIPLY)           \
    MACRO(BINARY_DIVIDE)             \
    MACRO(BINARY_REMAINDER)          \
    MACRO(BINARY_ADD)                \
    MACRO(BINARY_SUBTRACT)           \
    MACRO(BINARY_LESS_THAN)          \
    MACRO(BINARY_GREATER_THAN)       \
    MACRO(BINARY_LESS_THAN_EQUAL)    \
    MACRO(BINARY_GREATER_THAN_EQUAL) \
    MACRO(BINARY_EQUAL)              \
    MACRO(BINARY_NOT_EQUAL)          \
    MACRO(BINARY_LOGICAL_AND)        \
    MACRO(BINARY_LOGICAL_OR)         \
                                     \
    MACRO(TYPE_CAST)

DECLARE_ENUM(LIST_EXPRESSIONS, ExpressionKind, expression_kind)

struct Expression
{
    ExpressionKind kind;
    substr span;
    union
    {
        struct // IDENTITY_LITERAL
        {
            substr identity;
            bool given_error;
        };
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
            substr string_value;
        };
        struct // ENUM_VALUE_LITERAL
        {
            EnumValue *enum_value;
        };
        struct // VARIABLE_REFERENCE
        {
            Variable *variable;
        };
        struct // FUNCTION_REFERENCE
        {
            Function *function;
        };
        struct // PARAMETER_REFERENCE
        {
            Parameter *parameter;
        };
        struct // TYPE_REFERENCE
        {
            RhinoType type;
        };
        struct // FUNCTION_CALL
        {
            Expression *callee;
            ArgumentList arguments;
        };
        struct // INDEX_BY_FIELD
        {
            Expression *subject;
            substr field;
        };
        struct // RANGE_LITERAL
        {
            Expression *first;
            Expression *last;
        };
        struct // NONEABLE_EXPRESSION
        {
            Expression *__noneable__subject; // NOTE: KEEP SYNCED WITH INDEX_BY_FIELD subject
        };
        struct // UNARY_*
        {
            Expression *operand;
        };
        struct // BINARY_*
        {
            Expression *lhs;
            Expression *rhs;
        };
        struct // TYPE_CAST
        {
            Expression *cast_expr;
            RhinoType cast_type;
        };
    };
};

// Statement
#define LIST_STATEMENTS(MACRO)     \
    MACRO(INVALID_STATEMENT)       \
                                   \
    MACRO(FUNCTION_DECLARATION)    \
    MACRO(ENUM_TYPE_DECLARATION)   \
    MACRO(STRUCT_TYPE_DECLARATION) \
    MACRO(VARIABLE_DECLARATION)    \
                                   \
    MACRO(CODE_BLOCK)              \
                                   \
    MACRO(IF_SEGMENT)              \
    MACRO(ELSE_IF_SEGMENT)         \
    MACRO(ELSE_SEGMENT)            \
                                   \
    MACRO(BREAK_LOOP)              \
    MACRO(FOR_LOOP)                \
    MACRO(WHILE_LOOP)              \
    MACRO(BREAK_STATEMENT)         \
                                   \
    MACRO(ASSIGNMENT_STATEMENT)    \
                                   \
    MACRO(OUTPUT_STATEMENT)        \
    MACRO(EXPRESSION_STMT)         \
    MACRO(RETURN_STATEMENT)

DECLARE_ENUM(LIST_STATEMENTS, StatementKind, statement_kind)

struct Statement
{
    StatementKind kind;
    substr span;
    union
    {
        struct // VARIABLE_DECLARATION
        {
            Variable *variable;
            Expression *type_expression;
            Expression *initial_value;
            bool has_valid_identity;
        };
        struct // FUNCTION_DECLARATION
        {
            Function *function;
        };
        struct // ENUM_TYPE_DECLARATION
        {
            EnumType *enum_type;
        };
        struct // STRUCT_TYPE_DECLARATION
        {
            StructType *struct_type;
        };
        struct // CODE_BLOCK
        {
            Block *block;
        };
        struct // IF_SEGMENT / ELSE_IF_SEGMENT / ELSE_SEGMENT / WHILE_LOOP
        {
            Block *body;
            Expression *condition;
            Statement *next;
        };
        struct // BREAK_LOOP
        {
            Block *__loop_body; // NOTE: KEEP SYNCED WITH IF_SEGMENT body
        };
        struct // FOR_LOOP
        {
            Block *__for_body; // NOTE: KEEP SYNCED WITH IF_SEGMENT body
            Variable *iterator;
            Expression *iterable;
        };
        struct // ASSIGNMENT_STATEMENT
        {
            Expression *assignment_lhs;
            Expression *assignment_rhs;
        };
        struct // OUTPUT_STATEMENT / EXPRESSION_STMT / RETURN_STATEMENT
        {
            Expression *expression;
        };
    };
};

// Block

struct Block
{
    bool declaration_block;
    bool singleton_block;
    SymbolTable *symbol_table;
    StatementList statements;
};

// Function
struct Function
{
    substr span;
    substr identity;
    Block *body;

    Expression *return_type_expression;
    bool has_return_type_expression;
    RhinoType return_type;

    ParameterList parameters;
};

// Parameter
struct Parameter
{
    substr span;
    substr identity;
    Expression *type_expression;
    RhinoType type;
};

// Argument
struct Argument
{
    Expression *expr;
};

// Program
struct Program
{
    NativeType none_type;
    NativeType bool_type;
    NativeType int_type;
    NativeType num_type;
    NativeType str_type;

    Function *main;
    Block *program_block;
    SymbolTable *global_symbol_table;
};

void init_program(Program *apm, Allocator *allocator);

// Display APM
const char *rhino_type_string(Program *apm, RhinoType ty);
void print_parsed_apm(Program *apm, const char *source_text);
void print_resolved_apm(Program *apm, const char *source_text);

// Statement methods
bool is_declaration(Statement *stmt);

// Type analysis methods
RhinoType get_expression_type(Program *apm, const char *source_text, Expression *expr);
bool are_types_equal(RhinoType a, RhinoType b);
bool is_native_type(RhinoType ty, NativeType *native_type);
bool allow_assign_a_to_b(Program *apm, RhinoType a, RhinoType b);

#define IS_NONE_TYPE(ty) is_native_type(ty, &apm->none_type)
#define IS_BOOL_TYPE(ty) is_native_type(ty, &apm->bool_type)
#define IS_INT_TYPE(ty) is_native_type(ty, &apm->int_type)
#define IS_NUM_TYPE(ty) is_native_type(ty, &apm->num_type)
#define IS_STR_TYPE(ty) is_native_type(ty, &apm->str_type)

// Expression precedence methods
ExprPrecedence precedence_of(ExpressionKind expr_kind);

#endif
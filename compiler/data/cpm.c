#include "cpm.h"

// ENUMS //

DEFINE_ENUM(LIST_C_TYPE_TAG, CTypeTag, c_type_tag)
DEFINE_ENUM(LIST_C_EXPRESSIONS, CExpressionKind, c_expression_kind)
DEFINE_ENUM(LIST_C_STATEMENTS, CStatementKind, c_statement_kind)

// ALLOCATORS //

DEFINE_LIST_ALLOCATOR(CEnumValue, c_enum_value)
DEFINE_LIST_ALLOCATOR(CEnumType, c_enum_type)

DEFINE_LIST_ALLOCATOR(CVariable, c_variable)

DEFINE_LIST_ALLOCATOR(CVarDeclaration, c_var_declaration)

DEFINE_LIST_ALLOCATOR(CExpression, c_expression)

DEFINE_LIST_ALLOCATOR(CStatement, c_statement)

DEFINE_LIST_ALLOCATOR(CBlock, c_block)

DEFINE_LIST_ALLOCATOR(CFunction, c_function)
DEFINE_LIST_ALLOCATOR(CParameter, c_parameter)
DEFINE_LIST_ALLOCATOR(CArgument, c_argument)

// PROGRAM //

void init_c_program(CProgram *cpm, Allocator *allocator)
{
    init_allocator(&cpm->statement_lists, allocator, 1024);
    init_allocator(&cpm->parameter_lists, allocator, 1024);
    init_allocator(&cpm->argument_lists, allocator, 1024);

    init_c_enum_value_list_allocator(&cpm->enum_value, allocator, 1024);
    init_c_enum_type_list_allocator(&cpm->enum_type, allocator, 1024);
    init_c_variable_list_allocator(&cpm->variable, allocator, 1024);
    init_c_expression_list_allocator(&cpm->expression, allocator, 1024);
    init_c_statement_list_allocator(&cpm->statement, allocator, 1024);
    init_c_block_list_allocator(&cpm->block, allocator, 1024);
    init_c_function_list_allocator(&cpm->function, allocator, 1024);
    init_c_function_list_allocator(&cpm->native_function, allocator, 1024);
}
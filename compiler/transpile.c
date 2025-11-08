#include "transpile.h"
#include "fatal_error.h"
#include <assert.h>

#include <stdio.h>

// FORWARD DECLARATIONS //

void transpile_declaration_block(Compiler *c, Program *apm, CProgram *cpm, Block *block);
CBlock *transpile_code_block(Compiler *c, Program *apm, CProgram *cpm, Block *block);

// CONVERSION TABLES //

// FIXME: Make these hash-maps so that we can increase loop up speed

// Variable lookup table

typedef struct
{
    Variable *rhino;
    CVariable *c;
} VariableTableRow;

size_t variable_table_count = 0;
VariableTableRow *variable_table = NULL;

CVariable *lookup_c_variable(Variable *var)
{
    for (size_t i = 0; i < variable_table_count; i++)
        if (variable_table[i].rhino == var)
            return variable_table[i].c;

    fatal_error("Could not find Rhino variable %p in the variable table.", var);
    return NULL;
}

// Enum lookup tables

typedef struct
{
    EnumType *rhino;
    CEnumType *c;
} EnumTypeTableRow;

EnumTypeTableRow *enum_type_table = NULL;
size_t enum_type_table_count = 0;

typedef struct
{
    EnumValue *rhino;
    CEnumValue *c;
} EnumValueTableRow;

EnumValueTableRow *enum_value_table = NULL;
size_t enum_value_table_count = 0;

CEnumType *lookup_c_enum_type(EnumType *enum_type)
{
    for (size_t i = 0; i < enum_type_table_count; i++)
        if (enum_type_table[i].rhino == enum_type)
            return enum_type_table[i].c;

    fatal_error("Could not find Rhino enum type %p in the enum type table.", enum_type);
    return NULL;
}

CEnumValue *lookup_c_enum_value(EnumValue *enum_value)
{
    for (size_t i = 0; i < enum_value_table_count; i++)
        if (enum_value_table[i].rhino == enum_value)
            return enum_value_table[i].c;

    fatal_error("Could not find Rhino enum value %p in the enum value table.", enum_value);
    return NULL;
}

// UTILITY METHODS //

// FIXME: Don't recreate a bunch of expressions for the same literals over and over again
CExpression *get_int_literal(CProgram *cpm, int value)
{
    CExpression *literal = append_c_expression(&cpm->expression);
    literal->kind = C_INTEGER_LITERAL;
    literal->integer_value = value;
    return literal;
}

// TODO: Implement this correctly.
//       We are going to have to ditch substr for this?
CExpression *get_str_literal(CProgram *cpm, const char *str)
{
    CExpression *literal = append_c_expression(&cpm->expression);
    literal->kind = C_STRING_LITERAL;
    literal->string_value = str;
    return literal;
}

CExpression *get_substr_literal(CProgram *cpm, substr str)
{
    CExpression *literal = append_c_expression(&cpm->expression);
    literal->kind = C_SOURCE_STRING_LITERAL;
    literal->source_string = str;
    return literal;
}

// TYPES //

CType transpile_type(Compiler *c, Program *apm, CProgram *cpm, RhinoType rhino)
{
    switch (rhino.tag)
    {
    case RHINO_NATIVE_TYPE:
        if (IS_BOOL_TYPE(rhino))
            return (CType){C_BOOL, false, NULL};
        else if (IS_INT_TYPE(rhino))
            return (CType){C_INT, false, NULL};
        else if (IS_NUM_TYPE(rhino))
            return (CType){C_FLOAT, false, NULL};
        else if (IS_STR_TYPE(rhino))
            return (CType){C_STRING, false, NULL};

        // FIXME: This is incorrect! "None" in Rhino does not directly correspond to void
        else if (IS_NONE_TYPE(rhino))
            return (CType){C_VOID, false, NULL};

        else
            fatal_error("Unable to transpile native Rhino type %s.", rhino_type_string(apm, rhino));
        break;

    case RHINO_ENUM_TYPE:
        return (CType){C_ENUM_TYPE, false, lookup_c_enum_type(rhino.enum_type)};

    default:
        break;
    }

    fatal_error("Unable to transpile Rhino type %s.", rhino_type_string(apm, rhino));
    __builtin_unreachable();
}

// EXPRESSIONS //

CExpression *transpile_expression(Compiler *c, Program *apm, CProgram *cpm, Expression *rhino)
{
    assert(rhino != NULL);

    switch (rhino->kind)
    {
    case INTEGER_LITERAL:
        return get_int_literal(cpm, rhino->integer_value);

    case STRING_LITERAL:
        return get_substr_literal(cpm, rhino->string_value);

    case VARIABLE_REFERENCE:
    {
        CExpression *reference = append_c_expression(&cpm->expression);
        reference->kind = C_VARIABLE_REFERENCE;
        reference->variable = lookup_c_variable(rhino->variable);
        break;
    }

#define CASE_BINARY(expr_kind)                                     \
    case expr_kind:                                                \
    {                                                              \
        CExpression *expr = append_c_expression(&cpm->expression); \
        expr->kind = C_##expr_kind;                                \
        expr->lhs = transpile_expression(c, apm, cpm, rhino->lhs); \
        expr->rhs = transpile_expression(c, apm, cpm, rhino->rhs); \
        break;                                                     \
    }

        CASE_BINARY(BINARY_MULTIPLY)
        CASE_BINARY(BINARY_DIVIDE)
        CASE_BINARY(BINARY_REMAINDER)
        CASE_BINARY(BINARY_ADD)
        CASE_BINARY(BINARY_SUBTRACT)
        CASE_BINARY(BINARY_LESS_THAN)
        CASE_BINARY(BINARY_GREATER_THAN)
        CASE_BINARY(BINARY_LESS_THAN_EQUAL)
        CASE_BINARY(BINARY_GREATER_THAN_EQUAL)
        CASE_BINARY(BINARY_EQUAL)
        CASE_BINARY(BINARY_NOT_EQUAL)
        CASE_BINARY(BINARY_LOGICAL_AND)
        CASE_BINARY(BINARY_LOGICAL_OR)

#undef CASE_BINARY

    default:
        fatal_error("Could not transpile %s expression.", expression_kind_string(rhino->kind));
        break;
    }

    __builtin_unreachable();
}

// BLOCKS //

void transpile_declaration_block(Compiler *c, Program *apm, CProgram *cpm, Block *block)
{
    assert(block->declaration_block);

    Statement *rhino;
    StatementIterator it = statement_iterator(block->statements);
    while (rhino = next_statement_iterator(&it))
    {
        switch (rhino->kind)
        {

        case FUNCTION_DECLARATION:
        {
            Function *funct = rhino->function;
            CFunction *c_funct = append_c_function(&cpm->function);

            c_funct->identity = (CIdentity){.from_source = true, .sub = funct->identity};
            c_funct->return_type = transpile_type(c, apm, cpm, funct->return_type);
            c_funct->body = transpile_code_block(c, apm, cpm, funct->body);
            // FIXME: Transpile parameters

            break;
        }

        case ENUM_TYPE_DECLARATION:
            break;

        default:
            fatal_error("Could not transpile %s statement in declaration block.", statement_kind_string(rhino->kind));
            break;
        }
    }
}

CBlock *transpile_code_block(Compiler *c, Program *apm, CProgram *cpm, Block *block)
{
    assert(!block->declaration_block);

    CStatementListAllocator statement_allocator;
    init_c_statement_list_allocator(&statement_allocator, &cpm->statement_lists, 512); // FIXME: 512 was chosen arbitrarily

    Statement *rhino;
    StatementIterator it = statement_iterator(block->statements);
    while (rhino = next_statement_iterator(&it))
    {
        switch (rhino->kind)
        {

        case VARIABLE_DECLARATION:
        {
            CStatement *stmt = append_c_statement(&statement_allocator);
            stmt->kind = C_VARIABLE_DECLARATION;

            stmt->var_declaration.variable = lookup_c_variable(rhino->variable);
            stmt->var_declaration.initial_value = (rhino->initial_value) ? transpile_expression(c, apm, cpm, rhino->initial_value) : NULL;
            break;
        }

        case IF_SEGMENT:
        {
            CStatement *stmt = append_c_statement(&statement_allocator);
            stmt->kind = C_IF_SEGMENT;
            stmt->condition = transpile_expression(c, apm, cpm, rhino->condition);
            stmt->body = transpile_code_block(c, apm, cpm, rhino->body);
            break;
        }

        case ELSE_IF_SEGMENT:
        {
            CStatement *stmt = append_c_statement(&statement_allocator);
            stmt->kind = C_ELSE_IF_SEGMENT;
            stmt->condition = transpile_expression(c, apm, cpm, rhino->condition);
            stmt->body = transpile_code_block(c, apm, cpm, rhino->body);
            break;
        }

        case ELSE_SEGMENT:
        {
            CStatement *stmt = append_c_statement(&statement_allocator);
            stmt->kind = C_ELSE_SEGMENT;
            stmt->body = transpile_code_block(c, apm, cpm, rhino->body);
            break;
        }

        case BREAK_LOOP:
        {
            CStatement *stmt = append_c_statement(&statement_allocator);
            stmt->kind = C_BREAK_LOOP;
            stmt->body = transpile_code_block(c, apm, cpm, rhino->body);
            break;
        }

        case WHILE_LOOP:
        {
            CStatement *stmt = append_c_statement(&statement_allocator);
            stmt->kind = C_WHILE_LOOP;
            stmt->condition = transpile_expression(c, apm, cpm, rhino->condition);
            stmt->body = transpile_code_block(c, apm, cpm, rhino->body);
            break;
        }

        case FOR_LOOP:
        {
            CStatement *for_loop = append_c_statement(&statement_allocator);
            for_loop->kind = C_FOR_LOOP;

            CVariable *c_iterator = lookup_c_variable(rhino->iterator);
            for_loop->init.variable = c_iterator;

            CExpression *c_iterator_reference = append_c_expression(&cpm->expression);
            c_iterator_reference->kind = C_VARIABLE_REFERENCE;
            c_iterator_reference->variable = c_iterator;

            Expression *iterable = rhino->iterable;
            if (iterable->kind == RANGE_LITERAL)
            {
                // Init statement
                c_iterator->type = (CType){C_INT};
                for_loop->init.initial_value = transpile_expression(c, apm, cpm, iterable->first);

                // Condition expression
                for_loop->condition = append_c_expression(&cpm->expression);
                for_loop->condition->kind = C_BINARY_LESS_THAN_EQUAL;
                for_loop->condition->lhs = c_iterator_reference;
                for_loop->condition->rhs = transpile_expression(c, apm, cpm, iterable->last);

                // Advance expression
                for_loop->advance = append_c_expression(&cpm->expression);
                for_loop->advance->kind = C_UNARY_PREFIX_INCREMENT;
                for_loop->advance->operand = c_iterator_reference;

                // Body
                for_loop->body = transpile_code_block(c, apm, cpm, rhino->body);

                break;
            }

            if (iterable->kind == TYPE_REFERENCE && iterable->type.tag == RHINO_ENUM_TYPE)
            {
                CExpression *type_reference = append_c_expression(&cpm->expression);
                type_reference->kind = C_TYPE_REFERENCE;
                type_reference->type.tag = C_ENUM_TYPE;
                type_reference->type.enum_type = lookup_c_enum_type(iterable->type.enum_type);

                // Init statement
                c_iterator->type = (CType){C_ENUM_TYPE, lookup_c_enum_type(iterable->type.enum_type)};
                for_loop->init.initial_value = append_c_expression(&cpm->expression);
                for_loop->init.initial_value->kind = C_TYPE_CAST;
                for_loop->init.initial_value->lhs = type_reference;
                for_loop->init.initial_value->rhs = get_int_literal(cpm, 0);

                // Condition expression
                for_loop->condition = append_c_expression(&cpm->expression);
                for_loop->condition->kind = C_BINARY_LESS_THAN;
                for_loop->condition->lhs = c_iterator_reference;
                for_loop->condition->rhs = get_int_literal(cpm, iterable->type.enum_type->values.count);

                // Advance expression
                CExpression *add = append_c_expression(&cpm->expression);
                add->kind = C_BINARY_ADD;
                add->lhs = c_iterator_reference;
                add->rhs = get_int_literal(cpm, 1);

                CExpression *cast = append_c_expression(&cpm->expression);
                cast->kind = C_TYPE_CAST;
                cast->lhs = type_reference;
                cast->rhs = add;

                CExpression *assign = append_c_expression(&cpm->expression);
                assign->kind = C_ASSIGNMENT;
                assign->lhs = c_iterator_reference;
                assign->rhs = cast;

                for_loop->advance = assign;

                // Body
                for_loop->body = transpile_code_block(c, apm, cpm, rhino->body);

                break;
            }

            fatal_error("Could not transpile for loop with %s iterable.", expression_kind_string(iterable->kind));
            break;
        }

        case OUTPUT_STATEMENT:
        {
            CExpression *expr = append_c_expression(&cpm->expression);
            expr->kind = C_FUNCTION_CALL;
            expr->callee = append_c_expression(&cpm->expression);
            expr->callee->kind = C_FUNCTION_REFERENCE;
            expr->callee->function = cpm->printf_function;

            CArgumentListAllocator arg_allocator;
            init_c_argument_list_allocator(&arg_allocator, &cpm->argument_lists, 512); // FIXME: 512 was chosen arbitrarily

            Expression *output_value = rhino->expression;
            RhinoType output_type = get_expression_type(apm, c->source_text, rhino->expression);
            if (IS_BOOL_TYPE(output_type))
            {
                CArgument *format = append_c_argument(&arg_allocator);
                format->expr = get_str_literal(cpm, "%s\\n");

                CArgument *value = append_c_argument(&arg_allocator);
                value->expr = append_c_expression(&cpm->expression);
                value->expr->kind = C_TERNARY_CONDITIONAL;
                value->expr->condition = transpile_expression(c, apm, cpm, output_value);
                value->expr->when_true = get_str_literal(cpm, "true");
                value->expr->when_false = get_str_literal(cpm, "false");
            }
            else if (IS_STR_TYPE(output_type))
            {
                // TODO: If the expression is a string literal, don't bother creating the format string
                // if (expr->kind == STRING_LITERAL)

                CArgument *format = append_c_argument(&arg_allocator);
                format->expr = get_str_literal(cpm, "%s\\n");

                CArgument *value = append_c_argument(&arg_allocator);
                value->expr = transpile_expression(c, apm, cpm, output_value);
            }
            else if (IS_INT_TYPE(output_type))
            {
                CArgument *format = append_c_argument(&arg_allocator);
                format->expr = get_str_literal(cpm, "%d\\n");

                CArgument *value = append_c_argument(&arg_allocator);
                value->expr = transpile_expression(c, apm, cpm, output_value);
            }

            // TODO: Implement num printing
            // else if (IS_NUM_TYPE(output_type))
            // {
            //     EMIT_OPEN_BRACE();

            //     EMIT("float_to_str(");
            //     generate_expression(t, cpm, expr);
            //     EMIT_LINE(");");

            //     EMIT_ESCAPED("printf(\"%s\\n\", __to_str_buffer);");

            //     EMIT_CLOSE_BRACE();
            // }

            else if (output_type.tag == RHINO_ENUM_TYPE)
            {
                CArgument *format = append_c_argument(&arg_allocator);
                format->expr = get_str_literal(cpm, "%s\\n");

                CArgumentListAllocator sub_arg_allocator;
                init_c_argument_list_allocator(&sub_arg_allocator, &cpm->argument_lists, 512); // FIXME: 512 was chosen arbitrarily

                CArgument *value = append_c_argument(&sub_arg_allocator);
                value->expr = transpile_expression(c, apm, cpm, output_value);

                CArgument *str = append_c_argument(&arg_allocator);
                str->expr = append_c_expression(&cpm->expression);
                str->expr->kind = C_FUNCTION_CALL;

                str->expr->callee = append_c_expression(&cpm->expression);
                str->expr->callee->kind = C_FUNCTION_REFERENCE;
                str->expr->callee->function = lookup_c_enum_type(output_type.enum_type)->to_string;

                str->expr->arguments = get_c_argument_list(sub_arg_allocator);
            }

            else
            {
                fatal_error("Unable to generate output statement for expression with type %s.", rhino_type_string(apm, output_type));
            }

            expr->arguments = get_c_argument_list(arg_allocator);

            CStatement *stmt = append_c_statement(&statement_allocator);
            stmt->kind = C_EXPRESSION_STMT;
            stmt->expr = expr;

            break;
        }

        default:
            fatal_error("Could not transpile %s statement in code block.", statement_kind_string(rhino->kind));
            break;
        }
    }

    CBlock *c_block = append_c_block(&cpm->block);
    c_block->statements = get_c_statement_list(statement_allocator);
    return c_block;
}

// TRANSPILE //

void transpile(Compiler *c, Program *apm, CProgram *cpm)
{
    // Create enum lookup tables
    {
        EnumType *rhino_type;
        EnumValue *rhino_value;
        EnumTypeIterator type_it;
        EnumValueIterator value_it;

        // Count number of enum values
        type_it = enum_type_iterator(get_enum_type_list(apm->enum_type));
        while (rhino_type = next_enum_type_iterator(&type_it))
            enum_value_table_count += rhino_type->values.count;

        // Create enum types and values
        enum_type_table_count = apm->enum_type.count;
        enum_type_table = (EnumTypeTableRow *)malloc(sizeof(EnumTypeTableRow) * enum_type_table_count);

        enum_value_table = (EnumValueTableRow *)malloc(sizeof(EnumValueTableRow) * enum_value_table_count);

        type_it = enum_type_iterator(get_enum_type_list(apm->enum_type));
        size_t i = 0;
        size_t j = 0;
        while (rhino_type = next_enum_type_iterator(&type_it))
        {
            CFunction *to_string = append_c_function(&cpm->function);

            CEnumType *c_type = append_c_enum_type(&cpm->enum_type);
            c_type->identity = rhino_type->identity;
            c_type->to_string = to_string;
            c_type->first = j;
            c_type->count = rhino_type->values.count;
            enum_type_table[i++] = (EnumTypeTableRow){rhino_type, c_type};

            // FIXME: Generate a new string for the functoin identity
            to_string->identity = (CIdentity){.from_source = false, .str = "foofofofofof"};
            to_string->return_type = (CType){C_STRING, true, NULL};

            // TODO: Create a parameter called "value" whose type is the enum type

            to_string->body = append_c_block(&cpm->block);

            CStatementListAllocator statement_allocator;
            init_c_statement_list_allocator(&statement_allocator, &cpm->statement_lists, 512); // FIXME: 512 was chosen arbitrarily

            // TODO: Create a switch statement

            to_string->body->statements = get_c_statement_list(statement_allocator);

            value_it = enum_value_iterator(rhino_type->values);
            while (rhino_value = next_enum_value_iterator(&value_it))
            {
                CEnumValue *c_value = append_c_enum_value(&cpm->enum_value);
                c_value->identity = rhino_value->identity;

                // TODO: Create a case in the switch statement for this value

                enum_value_table[j++] = (EnumValueTableRow){rhino_value, c_value};
            }
        }
    }

    // Create variable lookup table
    {
        variable_table_count = apm->variable.count;
        variable_table = (VariableTableRow *)malloc(sizeof(VariableTableRow) * variable_table_count);

        Variable *rhino;
        VariableIterator it = variable_iterator(get_variable_list(apm->variable));
        size_t i = 0;
        while (rhino = next_variable_iterator(&it))
        {
            CVariable *var = append_c_variable(&cpm->variable);
            var->identity = rhino->identity;
            var->type = transpile_type(c, apm, cpm, rhino->type);
            variable_table[i++] = (VariableTableRow){rhino, var};
        }
    }

    // Native functions
    cpm->printf_function = append_c_function(&cpm->native_function);
    cpm->printf_function->identity = (CIdentity){.from_source = false, .str = "printf"};
    cpm->printf_function->return_type = (CType){C_VOID, false, NULL};

    // Transpile program block
    transpile_declaration_block(c, apm, cpm, apm->program_block);
}
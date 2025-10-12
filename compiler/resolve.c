#include "resolve.h"
#include "fatal_error.h"

#include <stdio.h>

// DETERMINE MAIN FUNCTION //

void determine_main_function(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->function.count; i++)
    {
        Function *funct = get_function(apm->function, i);
        if (substr_is(c->source_text, funct->identity, "main"))
        {
            apm->main = i;
            return;
        }
    }

    substr str;
    str.pos = 0;
    str.len = 0;
    raise_compilation_error(c, NO_MAIN_FUNCTION, str);
}

// RESOLVE IDENTITY LITERALS //
// This is the first pass for resolving literals.
// Other identity literals may be resolved in later passes.

void resolve_identities_in_expression(Compiler *c, Program *apm, size_t expr_index, SymbolTable *symbol_table);
void resolve_identities_in_code_block(Compiler *c, Program *apm, size_t block_index);
void resolve_identities_in_function(Compiler *c, Program *apm, size_t funct_index, SymbolTable *symbol_table);
void resolve_identities_in_struct_type(Compiler *c, Program *apm, size_t struct_type_index, SymbolTable *symbol_table);
void resolve_identities_in_declaration_block(Compiler *c, Program *apm, size_t block_index);

void resolve_identities_in_expression(Compiler *c, Program *apm, size_t expr_index, SymbolTable *symbol_table)
{
    Expression *expr = get_expression(apm->expression, expr_index);

    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        break;

    case IDENTITY_LITERAL:
    {
        // Primitive types
        // TODO: Implement these as types declared in the global scope which are not allowed to be shadowed
        if (substr_is(c->source_text, expr->identity, "int"))
        {
            expr->kind = TYPE_REFERENCE;
            expr->type.sort = SORT_INT;
            break;
        }

        if (substr_is(c->source_text, expr->identity, "num"))
        {
            expr->kind = TYPE_REFERENCE;
            expr->type.sort = SORT_NUM;
            break;
        }

        if (substr_is(c->source_text, expr->identity, "str"))
        {
            expr->kind = TYPE_REFERENCE;
            expr->type.sort = SORT_STR;
            break;
        }

        // Search symbol table
        SymbolTable *current_table = symbol_table;
        while (true)
        {
            bool found_symbol = false;
            for (size_t i = 0; i < current_table->symbol_count; i++)
            {
                Symbol s = current_table->symbol[i];

                if (!substr_match(c->source_text, s.identity, expr->identity))
                    continue;

                switch (s.tag)
                {
                case VARIABLE_SYMBOL:
                    expr->kind = VARIABLE_REFERENCE;
                    expr->variable = s.index;
                    break;

                case PARAMETER_SYMBOL:
                    expr->kind = PARAMETER_REFERENCE;
                    expr->variable = s.index;
                    break;

                case FUNCTION_SYMBOL:
                    expr->kind = FUNCTION_REFERENCE;
                    expr->function = s.index;
                    break;

                case ENUM_TYPE_SYMBOL:
                    expr->kind = TYPE_REFERENCE;
                    expr->type.sort = SORT_ENUM;
                    expr->type.index = s.index;
                    break;

                case STRUCT_TYPE_SYMBOL:
                    expr->kind = TYPE_REFERENCE;
                    expr->type.sort = SORT_STRUCT;
                    expr->type.index = s.index;
                    break;

                default:
                    fatal_error("Could not resolve IDENTITY_LITERAL that mapped to %s symbol.", symbol_tag_string(s.tag));
                }

                found_symbol = true;
                break;
            }

            if (found_symbol)
                break;

            if (current_table->next == 0)
                break;

            current_table = get_symbol_table(apm->symbol_table, current_table->next);
        }

        break;
    }

    case INTEGER_LITERAL:
    case FLOAT_LITERAL:
    case BOOLEAN_LITERAL:
    case STRING_LITERAL:
    case ENUM_VALUE_LITERAL:
        break;

    case VARIABLE_REFERENCE:
    case FUNCTION_REFERENCE:
    case TYPE_REFERENCE:
        break;

    case FUNCTION_CALL:
    {
        resolve_identities_in_expression(c, apm, expr->callee, symbol_table);

        Expression *callee = get_expression(apm->expression, expr->callee);
        if (callee->kind == IDENTITY_LITERAL)
        {
            raise_compilation_error(c, FUNCTION_DOES_NOT_EXIST, callee->span);
            callee->given_error = true;
        }
        else if (callee->kind != FUNCTION_REFERENCE)
        {
            raise_compilation_error(c, EXPRESSION_IS_NOT_A_FUNCTION, callee->span);
        }

        break;
    }

    case INDEX_BY_FIELD:
        resolve_identities_in_expression(c, apm, expr->subject, symbol_table);
        break;

    case RANGE_LITERAL:
        resolve_identities_in_expression(c, apm, expr->first, symbol_table);
        resolve_identities_in_expression(c, apm, expr->last, symbol_table);
        break;

    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        resolve_identities_in_expression(c, apm, expr->operand, symbol_table);
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
        resolve_identities_in_expression(c, apm, expr->lhs, symbol_table);
        resolve_identities_in_expression(c, apm, expr->rhs, symbol_table);
        break;

    default:
        fatal_error("Could not resolve identity literals in %s expression", expression_kind_string(expr->kind));
        break;
    }
}

// TODO: Strictly speaking, I'm not sure this function needs to use recursive decent.
//       It may be possible to iterate over the entire apm->statements array in order,
//       switching out the 'active' symbol table as appropriate.
void resolve_identities_in_code_block(Compiler *c, Program *apm, size_t block_index)
{
    Statement *block = get_statement(apm->statement, block_index);
    assert(block->kind == CODE_BLOCK || block->kind == SINGLE_BLOCK);

    SymbolTable *symbol_table = get_symbol_table(apm->symbol_table, block->symbol_table);
    size_t n;

    // Add all functions to symbol table. This allows functions to recursively refer to each other.
    n = get_first_statement_in_block(apm, block);
    while (n < block->statements.count)
    {
        Statement *stmt = get_statement(apm->statement, block->statements.first + n);

        if (stmt->kind == FUNCTION_DECLARATION)
        {
            Function *funct = get_function(apm->function, stmt->function);
            append_symbol(apm, block->symbol_table, FUNCTION_SYMBOL, n, funct->identity);
        }

        n = get_next_statement_in_block(apm, block, n);
    }

    // Sequentially resolve identities in each statement, adding variables and types to the symbol table as they are encountered
    n = get_first_statement_in_block(apm, block);
    while (n < block->statements.count)
    {
        Statement *stmt = get_statement(apm->statement, block->statements.first + n);

        switch (stmt->kind)
        {
        case INVALID_STATEMENT:
            break;

        case VARIABLE_DECLARATION:
        {
            if (stmt->has_initial_value)
                resolve_identities_in_expression(c, apm, stmt->initial_value, symbol_table);
            if (stmt->has_type_expression)
                resolve_identities_in_expression(c, apm, stmt->type_expression, symbol_table);

            Variable *var = get_variable(apm->variable, stmt->variable);
            append_symbol(apm, block->symbol_table, VARIABLE_SYMBOL, stmt->variable, var->identity);

            break;
        }

        case FUNCTION_DECLARATION:
            resolve_identities_in_function(c, apm, stmt->function, symbol_table);
            break;

        case ENUM_TYPE_DECLARATION:
        {
            EnumType *enum_type = get_enum_type(apm->enum_type, stmt->enum_type);
            append_symbol(apm, block->symbol_table, ENUM_TYPE_SYMBOL, stmt->enum_type, enum_type->identity);

            break;
        }

        case STRUCT_TYPE_DECLARATION:
        {
            resolve_identities_in_struct_type(c, apm, stmt->struct_type, symbol_table);

            StructType *struct_type = get_struct_type(apm->struct_type, stmt->struct_type);
            append_symbol(apm, block->symbol_table, STRUCT_TYPE_SYMBOL, stmt->struct_type, struct_type->identity);

            break;
        }

        case CODE_BLOCK:
        case SINGLE_BLOCK:
            resolve_identities_in_code_block(c, apm, block->statements.first + n);
            break;

        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
            resolve_identities_in_expression(c, apm, stmt->condition, symbol_table);
            resolve_identities_in_code_block(c, apm, stmt->body);
            break;

        case ELSE_SEGMENT:
            resolve_identities_in_code_block(c, apm, stmt->body);
            break;

        case BREAK_LOOP:
            resolve_identities_in_code_block(c, apm, stmt->body);
            break;

        case FOR_LOOP:
        {
            resolve_identities_in_expression(c, apm, stmt->iterable, symbol_table);

            Variable *iterator = get_variable(apm->variable, stmt->iterator);
            append_symbol(apm, block->symbol_table, VARIABLE_SYMBOL, stmt->iterator, iterator->identity);

            resolve_identities_in_code_block(c, apm, stmt->body);

            break;
        }

        case BREAK_STATEMENT:
            break;

        case ASSIGNMENT_STATEMENT:
            resolve_identities_in_expression(c, apm, stmt->assignment_lhs, symbol_table);
            resolve_identities_in_expression(c, apm, stmt->assignment_rhs, symbol_table);
            break;

        case OUTPUT_STATEMENT:
        case EXPRESSION_STMT:
        case RETURN_STATEMENT:
            resolve_identities_in_expression(c, apm, stmt->expression, symbol_table);
            break;

        default:
            fatal_error("Could not resolve identity literals in %s statement", statement_kind_string(stmt->kind));
            break;
        }

        n = get_next_statement_in_block(apm, block, n);
    }
}

void resolve_identities_in_function(Compiler *c, Program *apm, size_t funct_index, SymbolTable *symbol_table)
{
    Function *funct = get_function(apm->function, funct_index);

    if (funct->has_return_type_expression)
        resolve_identities_in_expression(c, apm, funct->return_type_expression, symbol_table);

    size_t body_symbol_table = get_statement(apm->statement, funct->body)->symbol_table;

    for (size_t i = 0; i < funct->parameters.count; i++)
    {
        Parameter *parameter = get_parameter_from_slice(apm->parameter, funct->parameters, i);
        resolve_identities_in_expression(c, apm, parameter->type_expression, symbol_table);
        append_symbol(apm, body_symbol_table, PARAMETER_SYMBOL, funct->parameters.first + i, parameter->identity);
    }

    resolve_identities_in_code_block(c, apm, funct->body);
}

void resolve_identities_in_struct_type(Compiler *c, Program *apm, size_t struct_type_index, SymbolTable *symbol_table)
{
    StructType *struct_type = get_struct_type(apm->struct_type, struct_type_index);

    for (size_t i = 0; i < struct_type->properties.count; i++)
    {
        Property *property = get_property_from_slice(apm->property, struct_type->properties, i);
        resolve_identities_in_expression(c, apm, property->type_expression, symbol_table);
    }
}

void resolve_identities_in_declaration_block(Compiler *c, Program *apm, size_t block_index)
{
    Statement *block = get_statement(apm->statement, block_index);
    assert(block->kind == DECLARATION_BLOCK);

    SymbolTable *symbol_table = get_symbol_table(apm->symbol_table, block->symbol_table);

    // NOTE: Currently, the only declaration block is the program scope
    //       and we add all symbols to the symbol table during parsing.
    //       Presumably, this may change at some point in the future?

    size_t n = get_first_statement_in_block(apm, block);
    while (n < block->statements.count)
    {
        Statement *stmt = get_statement(apm->statement, block->statements.first + n);

        if (stmt->kind == FUNCTION_DECLARATION)
            resolve_identities_in_function(c, apm, stmt->function, symbol_table);
        else if (stmt->kind == STRUCT_TYPE_DECLARATION)
            resolve_identities_in_struct_type(c, apm, stmt->function, symbol_table);

        n = get_next_statement_in_block(apm, block, n);
    }
}

// RESOLVE TYPES //

RhinoType resolve_type_expression(Compiler *c, Program *apm, size_t expr_index, SymbolTable *symbol_table);
void resolve_types_in_expression(Compiler *c, Program *apm, size_t expr_index, SymbolTable *symbol_table);
void resolve_types_in_code_block(Compiler *c, Program *apm, size_t block_index);
void resolve_types_in_function(Compiler *c, Program *apm, size_t funct_index, SymbolTable *symbol_table);
void resolve_types_in_struct_type(Compiler *c, Program *apm, size_t struct_type_index, SymbolTable *symbol_table);
void resolve_types_in_declaration_block(Compiler *c, Program *apm, size_t block_index);

RhinoType resolve_type_expression(Compiler *c, Program *apm, size_t expr_index, SymbolTable *symbol_table)
{
    Expression *expr = get_expression(apm->expression, expr_index);

    if (expr->kind == TYPE_REFERENCE)
        return expr->type;

    if (expr->kind == IDENTITY_LITERAL)
    {
        raise_compilation_error(c, TYPE_DOES_NOT_EXIST, expr->span);
        expr->given_error = true;
    }
    else
    {
        raise_compilation_error(c, TYPE_IS_INVALID, expr->span);
    }

    return (RhinoType){INVALID_SORT, 0};
}

void resolve_types_in_expression(Compiler *c, Program *apm, size_t expr_index, SymbolTable *symbol_table)
{
    Expression *expr = get_expression(apm->expression, expr_index);

    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        break;

    case IDENTITY_LITERAL:
        break;

    case INTEGER_LITERAL:
    case FLOAT_LITERAL:
    case BOOLEAN_LITERAL:
    case STRING_LITERAL:
    case ENUM_VALUE_LITERAL:
        break;

    case VARIABLE_REFERENCE:
    case FUNCTION_REFERENCE:
    case PARAMETER_REFERENCE:
    case TYPE_REFERENCE:
        break;

    case FUNCTION_CALL:
        resolve_types_in_expression(c, apm, expr->callee, symbol_table);
        break;

    case INDEX_BY_FIELD:
    {
        resolve_types_in_expression(c, apm, expr->subject, symbol_table);

        // Resolve enum values
        Expression *subject = get_expression(apm->expression, expr->subject);
        if (subject->kind != TYPE_REFERENCE)
            return;

        if (subject->type.sort != SORT_ENUM)
            return;

        EnumType *enum_type = get_enum_type(apm->enum_type, subject->type.index);

        size_t last = enum_type->values.first + enum_type->values.count - 1;
        for (size_t i = enum_type->values.first; i <= last; i++)
        {
            EnumValue *enum_value = get_enum_value(apm->enum_value, i);
            if (substr_match(c->source_text, expr->field, enum_value->identity))
            {
                expr->kind = ENUM_VALUE_LITERAL;
                expr->enum_value = i;
                return;
            }
        }

        raise_compilation_error(c, ENUM_VALUE_DOES_NOT_EXIST, expr->span);
        break;
    }

    case RANGE_LITERAL:
        resolve_types_in_expression(c, apm, expr->first, symbol_table);
        resolve_types_in_expression(c, apm, expr->last, symbol_table);
        break;

    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        resolve_types_in_expression(c, apm, expr->operand, symbol_table);
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
        resolve_types_in_expression(c, apm, expr->lhs, symbol_table);
        resolve_types_in_expression(c, apm, expr->rhs, symbol_table);
        break;

    default:
        fatal_error("Could not resolve types in %s expression", expression_kind_string(expr->kind));
        break;
    }
}

void resolve_types_in_code_block(Compiler *c, Program *apm, size_t block_index)
{
    Statement *block = get_statement(apm->statement, block_index);
    assert(block->kind == CODE_BLOCK || block->kind == SINGLE_BLOCK);

    SymbolTable *symbol_table = get_symbol_table(apm->symbol_table, block->symbol_table);
    size_t n = get_first_statement_in_block(apm, block);
    while (n < block->statements.count)
    {
        Statement *stmt = get_statement(apm->statement, block->statements.first + n);

        switch (stmt->kind)
        {
        case INVALID_STATEMENT:
            break;

        // Variable declarations, including type inference
        case VARIABLE_DECLARATION:
        {
            Variable *var = get_variable(apm->variable, stmt->variable);
            var->type.sort = INVALID_SORT;

            if (stmt->has_type_expression)
                var->type = resolve_type_expression(c, apm, stmt->type_expression, symbol_table);

            // FIXME: When the variable has a type expression, we should supply the type of
            //        the variable as a type hint when resolving types in the initial value.
            if (stmt->has_initial_value)
            {
                resolve_types_in_expression(c, apm, stmt->initial_value, symbol_table);

                if (!stmt->has_type_expression)
                    var->type = get_expression_type(apm, c->source_text, stmt->initial_value);
            }

            // TODO: What should happen if we find am inferred variable
            //       declaration that was defined without an initial value?

            break;
        }

        case FUNCTION_DECLARATION:
            resolve_types_in_function(c, apm, stmt->function, symbol_table);
            break;

        case ENUM_TYPE_DECLARATION:
            break;

        case STRUCT_TYPE_DECLARATION:
            resolve_types_in_struct_type(c, apm, stmt->struct_type, symbol_table);
            break;

        case CODE_BLOCK:
        case SINGLE_BLOCK:
            resolve_types_in_code_block(c, apm, block->statements.first + n);
            break;

        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
            resolve_types_in_expression(c, apm, stmt->condition, symbol_table);
            resolve_types_in_code_block(c, apm, stmt->body);
            break;

        case ELSE_SEGMENT:
            resolve_types_in_code_block(c, apm, stmt->body);
            break;

        case BREAK_LOOP:
            resolve_types_in_code_block(c, apm, stmt->body);
            break;

        // For loops, including type inference of the iterator
        case FOR_LOOP:
        {
            resolve_types_in_expression(c, apm, stmt->iterable, symbol_table);

            Variable *iterator = get_variable(apm->variable, stmt->iterator);
            Expression *iterable = get_expression(apm->expression, stmt->iterable);
            if (iterable->kind == RANGE_LITERAL)
            {
                iterator->type.sort = SORT_INT;
            }
            else if (iterable->kind == TYPE_REFERENCE && iterable->type.sort == SORT_ENUM)
            {
                iterator->type.sort = SORT_ENUM;
                iterator->type.index = iterable->type.index;
            }
            else
            {
                // FIXME: Generate an error
            }

            resolve_types_in_code_block(c, apm, stmt->body);
            break;
        }

        case BREAK_STATEMENT:
            break;

        case ASSIGNMENT_STATEMENT:
            resolve_types_in_expression(c, apm, stmt->assignment_lhs, symbol_table);
            resolve_types_in_expression(c, apm, stmt->assignment_rhs, symbol_table);
            break;

        case OUTPUT_STATEMENT:
        case EXPRESSION_STMT:
        case RETURN_STATEMENT:
            resolve_types_in_expression(c, apm, stmt->expression, symbol_table);
            break;

        default:
            fatal_error("Could not resolve types in %s statement", statement_kind_string(stmt->kind));
            break;
        }

        n = get_next_statement_in_block(apm, block, n);
    }
}

void resolve_types_in_function(Compiler *c, Program *apm, size_t funct_index, SymbolTable *symbol_table)
{
    Function *funct = get_function(apm->function, funct_index);

    if (funct->has_return_type_expression)
        funct->return_type = resolve_type_expression(c, apm, funct->return_type_expression, symbol_table);
    else
        funct->return_type.sort = SORT_NONE;

    for (size_t i = 0; i < funct->parameters.count; i++)
    {
        Parameter *parameter = get_parameter_from_slice(apm->parameter, funct->parameters, i);
        parameter->type = resolve_type_expression(c, apm, parameter->type_expression, symbol_table);
    }

    resolve_types_in_code_block(c, apm, funct->body);
}

void resolve_types_in_struct_type(Compiler *c, Program *apm, size_t struct_type_index, SymbolTable *symbol_table)
{
    StructType *struct_type = get_struct_type(apm->struct_type, struct_type_index);

    for (size_t i = 0; i < struct_type->properties.count; i++)
    {
        Property *property = get_property_from_slice(apm->property, struct_type->properties, i);
        property->type = resolve_type_expression(c, apm, property->type_expression, symbol_table);
    }
}

void resolve_types_in_declaration_block(Compiler *c, Program *apm, size_t block_index)
{
    Statement *block = get_statement(apm->statement, block_index);
    assert(block->kind == DECLARATION_BLOCK);

    SymbolTable *symbol_table = get_symbol_table(apm->symbol_table, block->symbol_table);

    size_t n = get_first_statement_in_block(apm, block);
    while (n < block->statements.count)
    {
        Statement *stmt = get_statement(apm->statement, block->statements.first + n);

        if (stmt->kind == FUNCTION_DECLARATION)
            resolve_types_in_function(c, apm, stmt->function, symbol_table);
        if (stmt->kind == STRUCT_TYPE_DECLARATION)
            resolve_types_in_struct_type(c, apm, stmt->struct_type, symbol_table);

        n = get_next_statement_in_block(apm, block, n);
    }
}

// RESOLVE //

void resolve(Compiler *c, Program *apm)
{
    determine_main_function(c, apm);
    resolve_identities_in_declaration_block(c, apm, apm->program_block);
    resolve_types_in_declaration_block(c, apm, apm->program_block);
}
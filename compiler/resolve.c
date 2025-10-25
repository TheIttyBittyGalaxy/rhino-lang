#include "resolve.h"
#include "fatal_error.h"

#include <assert.h>

// DETERMINE MAIN FUNCTION //

void determine_main_function(Compiler *c, Program *apm)
{
    Statement *declaration;
    StatementIterator it = statement_iterator(apm->program_block->statements);
    while (declaration = next_statement_iterator(&it))
    {
        if (declaration->kind == FUNCTION_DECLARATION)
        {
            Function *funct = declaration->function;
            if (substr_is(c->source_text, funct->identity, "main"))
            {
                apm->main = funct;
                return;
            }
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

void resolve_identities_in_expression(Compiler *c, Program *apm, Expression *expr, SymbolTable *symbol_table);
void resolve_identities_in_code_block(Compiler *c, Program *apm, Block *block);
void resolve_identities_in_function(Compiler *c, Program *apm, Function *funct, SymbolTable *symbol_table);
void resolve_identities_in_struct_type(Compiler *c, Program *apm, StructType *struct_type, SymbolTable *symbol_table);
void resolve_identities_in_declaration_block(Compiler *c, Program *apm, Block *block);

void resolve_identities_in_expression(Compiler *c, Program *apm, Expression *expr, SymbolTable *symbol_table)
{
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
                    expr->variable = s.variable;
                    break;

                case PARAMETER_SYMBOL:
                    expr->kind = PARAMETER_REFERENCE;
                    expr->parameter = s.parameter;
                    break;

                case FUNCTION_SYMBOL:
                    expr->kind = FUNCTION_REFERENCE;
                    expr->function = s.function;
                    break;

                case ENUM_TYPE_SYMBOL:
                    expr->kind = TYPE_REFERENCE;
                    expr->type.sort = SORT_ENUM;
                    expr->type.enum_type = s.enum_type;
                    break;

                case STRUCT_TYPE_SYMBOL:
                    expr->kind = TYPE_REFERENCE;
                    expr->type.sort = SORT_STRUCT;
                    expr->type.struct_type = s.struct_type;
                    break;

                default:
                    fatal_error("Could not resolve IDENTITY_LITERAL that mapped to %s symbol.", symbol_tag_string(s.tag));
                }

                found_symbol = true;
                break;
            }

            if (found_symbol)
                break;

            if (current_table->next == NULL)
                break;

            current_table = current_table->next;
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

        Expression *callee = expr->callee;
        if (callee->kind == IDENTITY_LITERAL)
        {
            raise_compilation_error(c, FUNCTION_DOES_NOT_EXIST, callee->span);
            callee->given_error = true;
        }
        else if (callee->kind != FUNCTION_REFERENCE)
        {
            raise_compilation_error(c, EXPRESSION_IS_NOT_A_FUNCTION, callee->span);
        }

        Argument *arg;
        ArgumentIterator it = argument_iterator(expr->arguments);
        while (arg = next_argument_iterator(&it))
            resolve_identities_in_expression(c, apm, arg->expr, symbol_table);

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
    case UNARY_NOT:
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

void resolve_identities_in_code_block(Compiler *c, Program *apm, Block *block)
{
    assert(!block->declaration_block);

    Statement *stmt;
    StatementIterator it;

    // Add all functions to symbol table. This allows functions to recursively refer to each other.
    it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind != FUNCTION_DECLARATION)
            continue;

        Function *funct = stmt->function;
        declare_symbol(apm, block->symbol_table, FUNCTION_SYMBOL, funct, funct->identity);
    }

    // Sequentially resolve identities in each statement, adding variables and types to the symbol table as they are encountered
    it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        switch (stmt->kind)
        {
        case INVALID_STATEMENT:
            break;

        case VARIABLE_DECLARATION:
        {
            if (stmt->initial_value)
                resolve_identities_in_expression(c, apm, stmt->initial_value, block->symbol_table);
            if (stmt->type_expression)
                resolve_identities_in_expression(c, apm, stmt->type_expression, block->symbol_table);

            Variable *var = stmt->variable;
            declare_symbol(apm, block->symbol_table, VARIABLE_SYMBOL, stmt->variable, var->identity);

            break;
        }

        case FUNCTION_DECLARATION:
            resolve_identities_in_function(c, apm, stmt->function, block->symbol_table);
            break;

        case ENUM_TYPE_DECLARATION:
        {
            EnumType *enum_type = stmt->enum_type;
            declare_symbol(apm, block->symbol_table, ENUM_TYPE_SYMBOL, stmt->enum_type, enum_type->identity);

            break;
        }

        case STRUCT_TYPE_DECLARATION:
        {
            resolve_identities_in_struct_type(c, apm, stmt->struct_type, block->symbol_table);

            StructType *struct_type = stmt->struct_type;
            declare_symbol(apm, block->symbol_table, STRUCT_TYPE_SYMBOL, stmt->struct_type, struct_type->identity);

            break;
        }

        case CODE_BLOCK:
            resolve_identities_in_code_block(c, apm, stmt->block);
            break;

        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
            resolve_identities_in_expression(c, apm, stmt->condition, block->symbol_table);
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
            resolve_identities_in_expression(c, apm, stmt->iterable, block->symbol_table);

            Variable *iterator = stmt->iterator;
            declare_symbol(apm, block->symbol_table, VARIABLE_SYMBOL, stmt->iterator, iterator->identity);

            resolve_identities_in_code_block(c, apm, stmt->body);

            break;
        }

        case WHILE_LOOP:
            resolve_identities_in_expression(c, apm, stmt->condition, block->symbol_table);
            resolve_identities_in_code_block(c, apm, stmt->body);
            break;

        case BREAK_STATEMENT:
            break;

        case ASSIGNMENT_STATEMENT:
            resolve_identities_in_expression(c, apm, stmt->assignment_lhs, block->symbol_table);
            resolve_identities_in_expression(c, apm, stmt->assignment_rhs, block->symbol_table);
            break;

        case OUTPUT_STATEMENT:
        case EXPRESSION_STMT:
        case RETURN_STATEMENT:
        {
            if (stmt->expression)
                resolve_identities_in_expression(c, apm, stmt->expression, block->symbol_table);
            break;
        }

        default:
            fatal_error("Could not resolve identity literals in %s statement", statement_kind_string(stmt->kind));
            break;
        }
    }
}

void resolve_identities_in_function(Compiler *c, Program *apm, Function *funct, SymbolTable *symbol_table)
{
    if (funct->has_return_type_expression)
        resolve_identities_in_expression(c, apm, funct->return_type_expression, symbol_table);

    Parameter *parameter;
    ParameterIterator it = parameter_iterator(funct->parameters);
    while (parameter = next_parameter_iterator(&it))
    {
        resolve_identities_in_expression(c, apm, parameter->type_expression, symbol_table);
        declare_symbol(apm, funct->body->symbol_table, PARAMETER_SYMBOL, parameter, parameter->identity);
    }

    resolve_identities_in_code_block(c, apm, funct->body);
}

void resolve_identities_in_struct_type(Compiler *c, Program *apm, StructType *struct_type, SymbolTable *symbol_table)
{
    Property *property;
    PropertyIterator it = property_iterator(struct_type->properties);
    while (property = next_property_iterator(&it))
        resolve_identities_in_expression(c, apm, property->type_expression, struct_type->body->symbol_table);
}

void resolve_identities_in_declaration_block(Compiler *c, Program *apm, Block *block)
{
    assert(block->declaration_block);

    // NOTE: Currently, the only declaration block is the program scope
    //       and we add all symbols to the symbol table during parsing.
    //       Presumably, this may change at some point in the future?

    Statement *stmt;
    StatementIterator it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind == FUNCTION_DECLARATION)
            resolve_identities_in_function(c, apm, stmt->function, block->symbol_table);
        else if (stmt->kind == STRUCT_TYPE_DECLARATION)
            resolve_identities_in_struct_type(c, apm, stmt->struct_type, block->symbol_table);
        else if (stmt->kind == VARIABLE_DECLARATION)
        {
            // TODO: This is copy/pasted and modified from resolve_identities_in_code_block.
            //       Find an effective way to tidy.
            if (stmt->initial_value)
                resolve_identities_in_expression(c, apm, stmt->initial_value, block->symbol_table);
            if (stmt->type_expression)
                resolve_identities_in_expression(c, apm, stmt->type_expression, block->symbol_table);
        }
    }
}

// RESOLVE TYPES //

RhinoType resolve_type_expression(Compiler *c, Program *apm, Expression *expr, SymbolTable *symbol_table);
void resolve_types_in_expression(Compiler *c, Program *apm, Expression *expr, SymbolTable *symbol_table, RhinoType type_hint);
void resolve_types_in_code_block(Compiler *c, Program *apm, Block *block);
void resolve_types_in_function(Compiler *c, Program *apm, Function *funct, SymbolTable *symbol_table);
void resolve_types_in_struct_type(Compiler *c, Program *apm, StructType *struct_type, SymbolTable *symbol_table);
void resolve_types_in_declaration_block(Compiler *c, Program *apm, Block *block);

RhinoType resolve_type_expression(Compiler *c, Program *apm, Expression *expr, SymbolTable *symbol_table)
{
    // FIXME: At the moment `parse_expression` will give a "expected expression" error whenever a type
    //        expression is invalid. Find a way of handling this that gives more helpful/specific errors.
    if (expr->kind == INVALID_EXPRESSION)
        return (RhinoType){ERROR_SORT, 0};

    if (expr->kind == TYPE_REFERENCE)
        return expr->type;

    if (expr->kind == IDENTITY_LITERAL)
    {
        raise_compilation_error(c, TYPE_DOES_NOT_EXIST, expr->span);
        expr->given_error = true;
        return (RhinoType){ERROR_SORT, 0};
    }

    if (expr->kind == INDEX_BY_FIELD)
    {
        RhinoType subject_type = resolve_type_expression(c, apm, expr->subject, symbol_table);

        if (subject_type.sort == ERROR_SORT)
            return (RhinoType){ERROR_SORT, 0}; // Return immediately, no need to produce an additional error

        if (subject_type.sort == SORT_STRUCT)
        {
            StructType *struct_type = subject_type.struct_type;
            SymbolTable *current_table = struct_type->body->symbol_table;

            while (true)
            {
                for (size_t i = 0; i < current_table->symbol_count; i++)
                {
                    Symbol s = current_table->symbol[i];

                    if (!substr_match(c->source_text, s.identity, expr->field))
                        continue;

                    switch (s.tag)
                    {
                    case ENUM_TYPE_SYMBOL:
                        expr->kind = TYPE_REFERENCE;
                        expr->type.sort = SORT_ENUM;
                        expr->type.enum_type = s.enum_type;
                        return expr->type;
                        break;

                    case STRUCT_TYPE_SYMBOL:
                        expr->kind = TYPE_REFERENCE;
                        expr->type.sort = SORT_STRUCT;
                        expr->type.struct_type = s.struct_type;
                        return expr->type;
                        break;

                    default:
                        fatal_error("Could not resolve IDENTITY_LITERAL that mapped to %s symbol inside of struct type.", symbol_tag_string(s.tag));
                    }
                }

                if (current_table->next == NULL)
                    break;

                current_table = current_table->next;
            }

            raise_compilation_error(c, TYPE_DOES_NOT_EXIST, expr->span);
            return (RhinoType){ERROR_SORT, 0};
        }
    }

    raise_compilation_error(c, TYPE_IS_INVALID, expr->span);
    return (RhinoType){ERROR_SORT, 0};
}

void resolve_types_in_expression(Compiler *c, Program *apm, Expression *expr, SymbolTable *symbol_table, RhinoType type_hint)
{
    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        break;

    case IDENTITY_LITERAL:
    {
        if (type_hint.sort == SORT_ENUM)
        {
            EnumType *enum_type = type_hint.enum_type;
            EnumValue *enum_value;
            EnumValueIterator it = enum_value_iterator(enum_type->values);
            while (enum_value = next_enum_value_iterator(&it))
            {
                if (substr_match(c->source_text, expr->identity, enum_value->identity))
                {
                    expr->kind = ENUM_VALUE_LITERAL;
                    expr->enum_value = enum_value;
                    return;
                }
            }
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
    case PARAMETER_REFERENCE:
    case TYPE_REFERENCE:
        break;

    case FUNCTION_CALL:
    {
        resolve_types_in_expression(c, apm, expr->callee, symbol_table, (RhinoType){SORT_NONE});

        // TODO: Use the parameter types as type hints for the arguments
        Argument *arg;
        ArgumentIterator it = argument_iterator(expr->arguments);
        while (arg = next_argument_iterator(&it))
            resolve_types_in_expression(c, apm, arg->expr, symbol_table, (RhinoType){SORT_NONE});

        break;
    }

    case INDEX_BY_FIELD:
    {
        resolve_types_in_expression(c, apm, expr->subject, symbol_table, (RhinoType){SORT_NONE});

        // Resolve enum values
        Expression *subject = expr->subject;
        if (subject->kind != TYPE_REFERENCE)
            return;

        if (subject->type.sort != SORT_ENUM)
            return;

        EnumType *enum_type = subject->type.enum_type;
        EnumValue *enum_value;
        EnumValueIterator it = enum_value_iterator(enum_type->values);
        while (enum_value = next_enum_value_iterator(&it))
        {
            if (substr_match(c->source_text, expr->field, enum_value->identity))
            {
                expr->kind = ENUM_VALUE_LITERAL;
                expr->enum_value = enum_value;
                return;
            }
        }

        raise_compilation_error(c, ENUM_VALUE_DOES_NOT_EXIST, expr->span);
        break;
    }

    case RANGE_LITERAL:
        resolve_types_in_expression(c, apm, expr->first, symbol_table, (RhinoType){SORT_NONE});
        resolve_types_in_expression(c, apm, expr->last, symbol_table, (RhinoType){SORT_NONE});
        break;

    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_NOT:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        resolve_types_in_expression(c, apm, expr->operand, symbol_table, (RhinoType){SORT_NONE});
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
        resolve_types_in_expression(c, apm, expr->lhs, symbol_table, (RhinoType){SORT_NONE});
        resolve_types_in_expression(c, apm, expr->rhs, symbol_table, (RhinoType){SORT_NONE});
        break;

    default:
        fatal_error("Could not resolve types in %s expression", expression_kind_string(expr->kind));
        break;
    }
}

void resolve_types_in_code_block(Compiler *c, Program *apm, Block *block)
{
    assert(!block->declaration_block);

    Statement *stmt;
    StatementIterator it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        switch (stmt->kind)
        {
        case INVALID_STATEMENT:
            break;

        // Variable declarations, including type inference
        case VARIABLE_DECLARATION:
        {
            Variable *var = stmt->variable;
            var->type.sort = INVALID_SORT;

            if (stmt->type_expression)
            {
                var->type = resolve_type_expression(c, apm, stmt->type_expression, block->symbol_table);

                if (stmt->initial_value)
                    resolve_types_in_expression(c, apm, stmt->initial_value, block->symbol_table, var->type);
            }
            else if (stmt->initial_value)
            {
                resolve_types_in_expression(c, apm, stmt->initial_value, block->symbol_table, (RhinoType){SORT_NONE});
                var->type = get_expression_type(apm, c->source_text, stmt->initial_value);
            }
            else
            {
                // TODO: What should happen if we find am inferred variable
                //       declaration that was defined without an initial value?
            }

            break;
        }

        case FUNCTION_DECLARATION:
            resolve_types_in_function(c, apm, stmt->function, block->symbol_table);
            break;

        case ENUM_TYPE_DECLARATION:
            break;

        case STRUCT_TYPE_DECLARATION:
            resolve_types_in_struct_type(c, apm, stmt->struct_type, block->symbol_table);
            break;

        case CODE_BLOCK:
            resolve_types_in_code_block(c, apm, stmt->block);
            break;

        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
            resolve_types_in_expression(c, apm, stmt->condition, block->symbol_table, (RhinoType){SORT_BOOL});
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
            resolve_types_in_expression(c, apm, stmt->iterable, block->symbol_table, (RhinoType){SORT_NONE});

            Variable *iterator = stmt->iterator;
            Expression *iterable = stmt->iterable;
            if (iterable->kind == RANGE_LITERAL)
            {
                iterator->type.sort = SORT_INT;
            }
            else if (iterable->kind == TYPE_REFERENCE && iterable->type.sort == SORT_ENUM)
            {
                iterator->type.sort = SORT_ENUM;
                iterator->type.enum_type = iterable->type.enum_type;
            }
            else
            {
                // FIXME: Generate an error
            }

            resolve_types_in_code_block(c, apm, stmt->body);
            break;
        }

        case WHILE_LOOP:
            resolve_types_in_expression(c, apm, stmt->condition, block->symbol_table, (RhinoType){SORT_BOOL});
            resolve_types_in_code_block(c, apm, stmt->body);
            break;

        case BREAK_STATEMENT:
            break;

        case ASSIGNMENT_STATEMENT:
        {
            resolve_types_in_expression(c, apm, stmt->assignment_lhs, block->symbol_table, (RhinoType){SORT_NONE});
            RhinoType lhs_type = get_expression_type(apm, c->source_text, stmt->assignment_lhs);
            resolve_types_in_expression(c, apm, stmt->assignment_rhs, block->symbol_table, lhs_type);
            break;
        }

        case OUTPUT_STATEMENT:
        case EXPRESSION_STMT:
        case RETURN_STATEMENT:
        {
            // TODO: Use the return type of the function as a type hint for return statements
            if (stmt->expression)
                resolve_types_in_expression(c, apm, stmt->expression, block->symbol_table, (RhinoType){SORT_NONE});
            break;
        }

        default:
            fatal_error("Could not resolve types in %s statement", statement_kind_string(stmt->kind));
            break;
        }
    }
}

void resolve_types_in_function(Compiler *c, Program *apm, Function *funct, SymbolTable *symbol_table)
{
    if (funct->has_return_type_expression)
        funct->return_type = resolve_type_expression(c, apm, funct->return_type_expression, symbol_table);
    else
        funct->return_type.sort = SORT_NONE;

    Parameter *parameter;
    ParameterIterator it = parameter_iterator(funct->parameters);
    while (parameter = next_parameter_iterator(&it))
    {
        parameter->type = resolve_type_expression(c, apm, parameter->type_expression, symbol_table);
    }

    resolve_types_in_code_block(c, apm, funct->body);
}

void resolve_types_in_struct_type(Compiler *c, Program *apm, StructType *struct_type, SymbolTable *symbol_table)
{
    Property *property;
    PropertyIterator it = property_iterator(struct_type->properties);
    while (property = next_property_iterator(&it))
        property->type = resolve_type_expression(c, apm, property->type_expression, struct_type->body->symbol_table);
}

void resolve_types_in_declaration_block(Compiler *c, Program *apm, Block *block)
{
    assert(block->declaration_block);

    Statement *stmt;
    StatementIterator it;

    // Resolve types in variable declarations
    it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind != VARIABLE_DECLARATION)
            continue;

        Variable *var = stmt->variable;

        if (stmt->type_expression)
            var->type = resolve_type_expression(c, apm, stmt->type_expression, block->symbol_table);

        if (stmt->initial_value)
            resolve_types_in_expression(c, apm, stmt->initial_value, block->symbol_table, var->type);
    }

    bool changes_made;
    do // For inferred type declarations, keep on resolving types until this no longer results in changes
    {
        changes_made = false;

        it = statement_iterator(block->statements);
        while (stmt = next_statement_iterator(&it))
        {
            if (stmt->kind == VARIABLE_DECLARATION && !stmt->type_expression && stmt->variable->type.sort == UNINITIALISED_SORT)
            {
                Variable *var = stmt->variable;

                if (stmt->initial_value)
                    var->type = get_expression_type(apm, c->source_text, stmt->initial_value);
                else
                    var->type.sort = ERROR_SORT;

                if (var->type.sort != UNINITIALISED_SORT)
                    changes_made = true;
            }
        }
    } while (changes_made);

    it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind == VARIABLE_DECLARATION && stmt->variable->type.sort == UNINITIALISED_SORT)
        {
            stmt->variable->type.sort = INVALID_SORT;
            // TODO: What should happen in this situation?
        }
    }

    // Resolve structs and functions
    it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind == FUNCTION_DECLARATION)
            resolve_types_in_function(c, apm, stmt->function, block->symbol_table);
        if (stmt->kind == STRUCT_TYPE_DECLARATION)
            resolve_types_in_struct_type(c, apm, stmt->struct_type, block->symbol_table);
    }
}

// RESOLVE VARIABLE ORDERS //

size_t determine_order_of_expression(Compiler *c, Program *apm, Expression *expr)
{
    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        return 0;

    case IDENTITY_LITERAL:
        return 0;

    case INTEGER_LITERAL:
    case FLOAT_LITERAL:
    case BOOLEAN_LITERAL:
    case STRING_LITERAL:
    case ENUM_VALUE_LITERAL:
        return 0;

    case VARIABLE_REFERENCE:
        return expr->variable->order;

    case FUNCTION_REFERENCE:
    case PARAMETER_REFERENCE:
    case TYPE_REFERENCE:
        return 0;

    case FUNCTION_CALL:
    {
        size_t largest_order = 0;
        Argument *arg;
        ArgumentIterator it = argument_iterator(expr->arguments);
        while (arg = next_argument_iterator(&it))
        {
            size_t order = determine_order_of_expression(c, apm, arg->expr);
            if (order > largest_order)
                largest_order = order;
        }
        return largest_order;
    }
    case INDEX_BY_FIELD:
        return determine_order_of_expression(c, apm, expr->subject);

    case RANGE_LITERAL:
    {
        size_t first = determine_order_of_expression(c, apm, expr->first);
        size_t last = determine_order_of_expression(c, apm, expr->last);
        return first > last ? first : last;
    }

    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_NOT:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        return determine_order_of_expression(c, apm, expr->operand);

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
    {
        size_t lhs = determine_order_of_expression(c, apm, expr->lhs);
        size_t rhs = determine_order_of_expression(c, apm, expr->rhs);
        return lhs > rhs ? lhs : rhs;
    }

    default:
        fatal_error("Could not determine order of %s expression", expression_kind_string(expr->kind));
        break;
    }

    unreachable();
}

// FIXME: This will loop forever if variables recursively refer to each other in their initial values
void resolve_variable_orders(Compiler *c, Program *apm, Block *block)
{
    assert(block->declaration_block);

    Statement *stmt;
    StatementIterator it;

    it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind != VARIABLE_DECLARATION)
            continue;

        stmt->variable->order = 0;
    }

    bool changes_made = true;
    while (changes_made)
    {
        changes_made = false;

        it = statement_iterator(block->statements);
        while (stmt = next_statement_iterator(&it))
        {
            if (stmt->kind == VARIABLE_DECLARATION && stmt->initial_value)
            {
                size_t order = determine_order_of_expression(c, apm, stmt->initial_value) + 1;
                if (order > stmt->variable->order)
                {
                    stmt->variable->order = order;
                    changes_made = true;
                }
            }
        }
    }
}

// RESOLVE //

void resolve(Compiler *c, Program *apm)
{
    determine_main_function(c, apm);
    resolve_identities_in_declaration_block(c, apm, apm->program_block);
    resolve_types_in_declaration_block(c, apm, apm->program_block);
    resolve_variable_orders(c, apm, apm->program_block); // FIXME: This will not tree-walk into nested declaration blocks
}
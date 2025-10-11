#include "resolve.h"
#include "fatal_error.h"

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

void resolve_identity_literals_in_expression(Compiler *c, Program *apm, size_t expr_index, SymbolTable *symbol_table)
{
    Expression *expr = get_expression(apm->expression, expr_index);

    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        break;

    case IDENTITY_LITERAL:
    {
        // Primitive types
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

                case ENUM_TYPE_SYMBOL:
                    expr->kind = TYPE_REFERENCE;
                    expr->type.sort = SORT_ENUM;
                    expr->type.index = s.index;
                    break;

                case FUNCTION_SYMBOL:
                    expr->kind = FUNCTION_REFERENCE;
                    expr->function = s.index;
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
        resolve_identity_literals_in_expression(c, apm, expr->callee, symbol_table);
        break;

    case INDEX_BY_FIELD:
        resolve_identity_literals_in_expression(c, apm, expr->subject, symbol_table);
        break;

    case RANGE_LITERAL:
        resolve_identity_literals_in_expression(c, apm, expr->first, symbol_table);
        resolve_identity_literals_in_expression(c, apm, expr->last, symbol_table);
        break;

    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        resolve_identity_literals_in_expression(c, apm, expr->operand, symbol_table);
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
        resolve_identity_literals_in_expression(c, apm, expr->lhs, symbol_table);
        resolve_identity_literals_in_expression(c, apm, expr->rhs, symbol_table);
        break;

    default:
        fatal_error("Could not resolve identity literals in %s expression", expression_kind_string(expr->kind));
        break;
    }
}

void resolve_identity_literals_in_block(Compiler *c, Program *apm, size_t block_index)
{
    Statement *block = get_statement(apm->statement, block_index);
    assert(block->kind == CODE_BLOCK || block->kind == SINGLE_BLOCK);

    SymbolTable *symbol_table = get_symbol_table(apm->symbol_table, block->symbol_table);

    size_t last = get_last_statement_in_code_block(apm, block);
    size_t n = get_first_statement_in_code_block(apm, block);
    while (n < block->statements.count)
    {
        Statement *stmt = get_statement(apm->statement, block->statements.first + n);
        switch (stmt->kind)
        {
        case INVALID_STATEMENT:
            break;

        case VARIABLE_DECLARATION:
        {
            Variable *var = get_variable(apm->variable, stmt->variable);
            append_symbol(apm, block->symbol_table, VARIABLE_SYMBOL, stmt->variable, var->identity);

            if (stmt->has_initial_value)
                resolve_identity_literals_in_expression(c, apm, stmt->initial_value, symbol_table);
            if (stmt->has_type_expression)
                resolve_identity_literals_in_expression(c, apm, stmt->type_expression, symbol_table);
            break;
        }

        case FUNCTION_DECLARATION:
            break;

        case ENUM_TYPE_DECLARATION:
            break;

        case CODE_BLOCK:
        case SINGLE_BLOCK:
            resolve_identity_literals_in_block(c, apm, block->statements.first + n);
            break;

        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
            resolve_identity_literals_in_expression(c, apm, stmt->condition, symbol_table);
            resolve_identity_literals_in_block(c, apm, stmt->body);
            break;

        case ELSE_SEGMENT:
            resolve_identity_literals_in_block(c, apm, stmt->body);
            break;

        case BREAK_LOOP:
            resolve_identity_literals_in_block(c, apm, stmt->body);
            break;

        case FOR_LOOP:
        {
            Variable *var = get_variable(apm->variable, stmt->iterator);
            append_symbol(apm, block->symbol_table, VARIABLE_SYMBOL, stmt->iterator, var->identity);

            resolve_identity_literals_in_expression(c, apm, stmt->iterable, symbol_table);
            resolve_identity_literals_in_block(c, apm, stmt->body);
            break;
        }

        case BREAK_STATEMENT:
            break;

        case ASSIGNMENT_STATEMENT:
            resolve_identity_literals_in_expression(c, apm, stmt->assignment_lhs, symbol_table);
            resolve_identity_literals_in_expression(c, apm, stmt->assignment_rhs, symbol_table);
            break;

        case OUTPUT_STATEMENT:
        case EXPRESSION_STMT:
        case RETURN_STATEMENT:
            resolve_identity_literals_in_expression(c, apm, stmt->expression, symbol_table);
            break;

        default:
            fatal_error("Could not resolve identity literals in %s statement", statement_kind_string(stmt->kind));
            break;
        }

        n = get_next_statement_in_code_block(apm, block, n);
    }
}

void resolve_identity_literals_in_function(Compiler *c, Program *apm, size_t funct_index)
{
    Function *funct = get_function(apm->function, funct_index);

    if (funct->has_return_type_expression)
    {
        SymbolTable *global_symbol_table = get_symbol_table(apm->symbol_table, apm->global_symbol_table);
        resolve_identity_literals_in_expression(c, apm, funct->return_type_expression, global_symbol_table);
    }

    resolve_identity_literals_in_block(c, apm, funct->body);
}

// RESOLVE APM NODES //

void resolve_enum_values(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->expression.count; i++)
    {
        Expression *index_by_field = get_expression(apm->expression, i);

        if (index_by_field->kind != INDEX_BY_FIELD)
            continue;

        Expression *subject = get_expression(apm->expression, index_by_field->subject);

        if (subject->kind != TYPE_REFERENCE)
            continue;

        if (subject->type.sort != SORT_ENUM)
            continue;

        EnumType *enum_type = get_enum_type(apm->enum_type, subject->type.index);

        bool found_enum_value = false;

        size_t last = enum_type->values.first + enum_type->values.count - 1;
        for (size_t i = enum_type->values.first; i <= last; i++)
        {
            EnumValue *enum_value = get_enum_value(apm->enum_value, i);
            if (substr_match(c->source_text, index_by_field->field, enum_value->identity))
            {
                index_by_field->kind = ENUM_VALUE_LITERAL;
                index_by_field->enum_value = i;
                found_enum_value = true;
                break;
            }
        }

        if (found_enum_value)
            continue;

        raise_compilation_error(c, ENUM_VALUE_DOES_NOT_EXIST, index_by_field->span);
    }
}

void resolve_function_calls(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->expression.count; i++)
    {
        Expression *funct_call = get_expression(apm->expression, i);

        if (funct_call->kind != FUNCTION_CALL)
            continue;

        Expression *callee_expr = get_expression(apm->expression, funct_call->callee);

        if (callee_expr->kind != FUNCTION_REFERENCE)
        {
            if (callee_expr->kind == IDENTITY_LITERAL)
            {
                raise_compilation_error(c, FUNCTION_DOES_NOT_EXIST, callee_expr->span);
                callee_expr->given_error = true;
            }
            else
            {
                raise_compilation_error(c, EXPRESSION_IS_NOT_A_FUNCTION, callee_expr->span);
            }
            continue;
        }

        funct_call->callee = callee_expr->function;
    }
}

void resolve_function_return_types(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->function.count; i++)
    {
        Function *funct = get_function(apm->function, i);

        if (!funct->has_return_type_expression)
        {
            funct->return_type.sort = SORT_NONE;
            continue;
        }

        // FIXME: This code is copy/pasted and modified from `infer_variable_types`
        //        Should there be a "resolve type expression" method?
        Expression *return_type_expression = get_expression(apm->expression, funct->return_type_expression);
        if (return_type_expression->kind == TYPE_REFERENCE)
        {
            funct->return_type = return_type_expression->type;
        }
        else if (return_type_expression->kind == IDENTITY_LITERAL)
        {
            raise_compilation_error(c, TYPE_DOES_NOT_EXIST, return_type_expression->span);
            return_type_expression->given_error = true;
        }
        else
        {
            raise_compilation_error(c, FUNCTION_RETURN_TYPE_IS_INVALID, return_type_expression->span);
        }
    }
}

// TYPE INFERENCE //

void infer_variable_types(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);

        // Variable declarations
        if (stmt->kind == VARIABLE_DECLARATION)
        {
            Variable *var = get_variable(apm->variable, stmt->variable);
            var->type.sort = INVALID_SORT;

            if (stmt->has_type_expression)
            {
                Expression *type_expression = get_expression(apm->expression, stmt->type_expression);
                if (type_expression->kind == TYPE_REFERENCE)
                {
                    var->type = type_expression->type;
                }
                else if (type_expression->kind == IDENTITY_LITERAL)
                {
                    raise_compilation_error(c, TYPE_DOES_NOT_EXIST, type_expression->span);
                    type_expression->given_error = true;
                }
                else
                {
                    raise_compilation_error(c, VARIABLE_TYPE_IS_INVALID, type_expression->span);
                }
            }
            else if (stmt->has_initial_value)
            {
                var->type = get_expression_type(apm, stmt->initial_value);
            }
            else
            {
                // TODO: What should we do here? Presumably we can only reach this state if the parser
                //       found an inferred variable declaration defined without an initial value?
            }
        }

        // FOR LOOP ITERATORS
        else if (stmt->kind == FOR_LOOP)
        {
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
                iterator->type.sort = INVALID_SORT;
                // FIXME: Generate an error
            }
        }
    }
}

// RESOLVE //

void resolve(Compiler *c, Program *apm)
{
    determine_main_function(c, apm);

    // resolve_identity_literals
    for (size_t i = 0; i < apm->function.count; i++)
        resolve_identity_literals_in_function(c, apm, i);

    resolve_enum_values(c, apm);
    resolve_function_calls(c, apm);
    resolve_function_return_types(c, apm);

    infer_variable_types(c, apm);

    // Produce errors for any remaining invalid identity literals
    {
        for (size_t i = 0; i < apm->expression.count; i++)
        {
            Expression *identity_literal = get_expression(apm->expression, i);
            if (identity_literal->kind == IDENTITY_LITERAL && !identity_literal->given_error)
            {
                raise_compilation_error(c, IDENTITY_DOES_NOT_EXIST, identity_literal->span);
                identity_literal->given_error = true;
            }
        }
    }
}
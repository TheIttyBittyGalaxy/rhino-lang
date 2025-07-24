#include "analyse.h"
#include "fatal_error.h"
#include <string.h>

// ANALYSIS STAGES //

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

// RESOLVE APM NODES //

void resolve_identity_literals_in_expression(Compiler *c, Program *apm, size_t expr_index, SymbolTable *symbol_table)
{
    Expression *expr = get_expression(apm->expression, expr_index);

    switch (expr->kind)
    {
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

        // Scan symbol table
        SymbolTable *current_table = symbol_table;
        bool found_symbol = false;
        while (true)
        {
            for (size_t i = 0; i < current_table->symbol_count; i++)
            {
                Symbol s = current_table->symbol[i];

                if (!substr_match(c->source_text, s.identity, expr->identity))
                    continue;

                if (s.tag == ENUM_TYPE_SYMBOL)
                {
                    expr->kind = TYPE_REFERENCE;
                    expr->type.sort = SORT_ENUM;
                    expr->type.index = s.index;
                    found_symbol = true;
                    break;
                }

                if (s.tag == FUNCTION_SYMBOL)
                {
                    expr->kind = FUNCTION_REFERENCE;
                    expr->function = s.index;
                    found_symbol = true;
                    break;
                }
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

void resolve_identity_literals(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *block = get_statement(apm->statement, i);

        if (block->kind != CODE_BLOCK && block->kind != SINGLE_BLOCK)
            continue;

        SymbolTable *symbol_table = get_symbol_table(apm->symbol_table, block->symbol_table);

        size_t last = get_last_statement_in_code_block(apm, block);
        size_t n = get_first_statement_in_code_block(apm, block);
        while (n < block->statements.count)
        {
            Statement *stmt = get_statement(apm->statement, block->statements.first + n);
            switch (stmt->kind)
            {
            case CODE_BLOCK:
            case SINGLE_BLOCK:
                break; // Nested blocks will be handled by the outer for loop of this function

            case IF_SEGMENT:
            case ELSE_IF_SEGMENT:
                resolve_identity_literals_in_expression(c, apm, stmt->condition, symbol_table);
                break;

            case ELSE_SEGMENT:
                break;

            case BREAK_LOOP:
                break;

            case FOR_LOOP:
                resolve_identity_literals_in_expression(c, apm, stmt->first, symbol_table);
                resolve_identity_literals_in_expression(c, apm, stmt->last, symbol_table);
                break;

            case BREAK_STATEMENT:
                break;

            case ASSIGNMENT_STATEMENT:
                resolve_identity_literals_in_expression(c, apm, stmt->assignment_lhs, symbol_table);
                resolve_identity_literals_in_expression(c, apm, stmt->assignment_rhs, symbol_table);
                break;

            case VARIABLE_DECLARATION:
                if (stmt->has_initial_value)
                    resolve_identity_literals_in_expression(c, apm, stmt->initial_value, symbol_table);
                if (stmt->has_type_expression)
                    resolve_identity_literals_in_expression(c, apm, stmt->type_expression, symbol_table);
                break;

            case OUTPUT_STATEMENT:
            case EXPRESSION_STMT:
                resolve_identity_literals_in_expression(c, apm, stmt->expression, symbol_table);
                break;

            case FUNCTION_DECLARATION:
                break;

            default:
                fatal_error("Could not resolve identity literals in %s statement", statement_kind_string(stmt->kind));
                break;
            }

            n = get_next_statement_in_code_block(apm, block, n);
        }
    }
}

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

void resolve_variable_types(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);

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
    }
}

// CHECK SEMANTICS //

void check_conditions_are_booleans(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);

        if (stmt->kind == IF_SEGMENT || stmt->kind == ELSE_IF_SEGMENT)
        {
            if (!is_expression_boolean(apm, stmt->condition))
            {
                Expression *condition = get_expression(apm->expression, stmt->condition);
                raise_compilation_error(c, CONDITION_IS_NOT_BOOLEAN, condition->span);
            }
        }
    }
}

void check_function_calls(Compiler *c, Program *apm)
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

void check_variable_assignments(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);

        if (stmt->kind == VARIABLE_DECLARATION)
        {
            if (!stmt->has_initial_value)
                continue;

            Variable *var = get_variable(apm->variable, stmt->variable);
            RhinoType value_type = get_expression_type(apm, stmt->initial_value);

            if (!can_assign_a_to_b(value_type, var->type))
                raise_compilation_error(c, RHS_TYPE_DOES_NOT_MATCH_LHS, stmt->span);
        }
        else if (stmt->kind == ASSIGNMENT_STATEMENT)
        {
            RhinoType lhs_type = get_expression_type(apm, stmt->assignment_lhs);
            RhinoType rhs_type = get_expression_type(apm, stmt->assignment_rhs);

            if (!can_assign_a_to_b(rhs_type, lhs_type))
                raise_compilation_error(c, RHS_TYPE_DOES_NOT_MATCH_LHS, stmt->span);
        }
    }
}

void check_invalid_identity_literals(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->expression.count; i++)
    {
        Expression *identity_literal = get_expression(apm->expression, i);

        if (identity_literal->kind != IDENTITY_LITERAL)
            continue;

        if (identity_literal->given_error)
            continue;

        raise_compilation_error(c, IDENTITY_DOES_NOT_EXIST, identity_literal->span);
    }
}

// ANALYSE //

void analyse(Compiler *c, Program *apm)
{
    determine_main_function(c, apm);

    resolve_identity_literals(c, apm);
    resolve_enum_values(c, apm);
    resolve_variable_types(c, apm);

    check_conditions_are_booleans(c, apm);
    check_function_calls(c, apm);
    check_variable_assignments(c, apm);

    check_invalid_identity_literals(c, apm);
}
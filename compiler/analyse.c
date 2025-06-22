#include "analyse.h"
#include <string.h>

// UTILITY METHODS //

bool is_expression_boolean(Program *apm, size_t expr_index)
{
    Expression *expr = get_expression(apm->expression, expr_index);
    if (expr->kind == BOOLEAN_LITERAL)
    {
        return true;
    }

    return false;
}

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

void resolve_types(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);

        // TODO: Correctly resolve the type of each variable declaration
        if (stmt->kind == VARIABLE_DECLARATION)
        {
            if (stmt->has_type_expression)
            {
                Expression *type_expression = get_expression(apm->expression, stmt->type_expression);
                if (type_expression->kind != IDENTITY_LITERAL)
                {
                    // FIXME: Really the error here should be something like "invalid type expression"
                    raise_compilation_error(c, VARIABLE_DECLARED_WITH_INVALID_TYPE, type_expression->span);
                }

                type_expression->identity_resolved = true;

                Variable *var = get_variable(apm->variable, stmt->variable);
                if (substr_is(c->source_text, type_expression->identity, "int"))
                    var->type = RHINO_INT;
                else if (substr_is(c->source_text, type_expression->identity, "num"))
                    var->type = RHINO_NUM;
                else if (substr_is(c->source_text, type_expression->identity, "str"))
                    var->type = RHINO_STR;
                else
                {
                    var->type = INVALID_RHINO_TYPE;
                    raise_compilation_error(c, VARIABLE_DECLARED_WITH_INVALID_TYPE, type_expression->span);
                }
            }
        }
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

        if (callee_expr->kind != IDENTITY_LITERAL)
        {
            raise_compilation_error(c, EXPRESSION_IS_NOT_A_FUNCTION, funct_call->span);
            continue;
        }

        callee_expr->identity_resolved = true;

        substr identity = callee_expr->identity;
        bool funct_exists = false;
        for (size_t i = 0; i < apm->function.count; i++)
        {
            Function *funct = get_function(apm->function, i);
            if (substr_match(c->source_text, funct->identity, identity))
            {
                funct_call->callee = i;
                funct_exists = true;
                break;
            }
        }

        if (!funct_exists)
            raise_compilation_error(c, FUNCTION_DOES_NOT_EXIST, funct_call->span);
    }
}

void resolve_identity_literals(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->expression.count; i++)
    {
        Expression *identity_literal = get_expression(apm->expression, i);

        if (identity_literal->kind != IDENTITY_LITERAL)
            continue;

        if (identity_literal->identity_resolved)
            continue;

        raise_compilation_error(c, VARIABLE_DOES_NOT_EXIST, identity_literal->span);
    }
}

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

// ANALYSE //

void analyse(Compiler *c, Program *apm)
{
    determine_main_function(c, apm);

    resolve_types(c, apm);
    resolve_function_calls(c, apm);
    resolve_identity_literals(c, apm);

    check_conditions_are_booleans(c, apm);
}
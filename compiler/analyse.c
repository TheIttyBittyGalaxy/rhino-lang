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

void resolve_identity_literals(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->expression.count; i++)
    {
        Expression *expr = get_expression(apm->expression, i);

        if (expr->kind == FUNCTION_CALL)
        {
            Expression *callee_expr = get_expression(apm->expression, expr->callee);

            if (callee_expr->kind != IDENTITY_LITERAL)
            {
                raise_compilation_error(c, EXPRESSION_IS_NOT_A_FUNCTION, expr->span);
                continue;
            }

            substr identity = callee_expr->identity;
            bool funct_exists = false;
            for (size_t i = 0; i < apm->function.count; i++)
            {
                Function *funct = get_function(apm->function, i);
                if (substr_match(c->source_text, funct->identity, identity))
                {
                    expr->callee = i;
                    funct_exists = true;
                    break;
                }
            }

            if (!funct_exists)
                raise_compilation_error(c, FUNCTION_DOES_NOT_EXIST, expr->span);
        }

        else if (expr->kind == IDENTITY_LITERAL)
        {
            raise_compilation_error(c, VARIABLE_DOES_NOT_EXIST, expr->span);
        }
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
    resolve_identity_literals(c, apm);
    check_conditions_are_booleans(c, apm);
}
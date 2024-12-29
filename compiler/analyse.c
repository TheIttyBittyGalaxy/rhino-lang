#include "analyse.h"
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

void resolve_function_calls(Compiler *c, Program *apm)
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
            {
                raise_compilation_error(c, FUNCTION_DOES_NOT_EXIST, expr->span);
            }
        }
    }
}

// ANALYSE //

void analyse(Compiler *c, Program *apm)
{
    determine_main_function(c, apm);
    resolve_function_calls(c, apm);
}
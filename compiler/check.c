#include "check.h"
#include "fatal_error.h"
#include <string.h>

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

void check_variable_assignments(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);

        if (stmt->kind == VARIABLE_DECLARATION && stmt->has_initial_value)
        {
            RhinoType var_type = get_variable(apm->variable, stmt->variable)->type;
            RhinoType value_type = get_expression_type(apm, stmt->initial_value);

            if (!allow_assign_a_to_b(value_type, var_type))
                raise_compilation_error(c, RHS_TYPE_DOES_NOT_MATCH_LHS, stmt->span);
        }

        else if (stmt->kind == ASSIGNMENT_STATEMENT)
        {
            RhinoType lhs_type = get_expression_type(apm, stmt->assignment_lhs);
            RhinoType rhs_type = get_expression_type(apm, stmt->assignment_rhs);

            if (!allow_assign_a_to_b(rhs_type, lhs_type))
                raise_compilation_error(c, RHS_TYPE_DOES_NOT_MATCH_LHS, stmt->span);
        }
    }
}

void produce_errors_for_invalid_identity_literals(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->expression.count; i++)
    {
        Expression *identity_literal = get_expression(apm->expression, i);

        if (identity_literal->kind != IDENTITY_LITERAL)
            continue;

        if (identity_literal->given_error)
            continue;

        raise_compilation_error(c, IDENTITY_DOES_NOT_EXIST, identity_literal->span);
        identity_literal->given_error = true;
    }
}

// ANALYSE //

void check(Compiler *c, Program *apm)
{
    check_conditions_are_booleans(c, apm);
    check_variable_assignments(c, apm);

    produce_errors_for_invalid_identity_literals(c, apm);
}
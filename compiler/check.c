#include "check.h"
#include "fatal_error.h"
#include <string.h>

// CHECK //

void check(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);

        switch (stmt->kind)
        {

        // Check if statement conditions are booleans
        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
        {
            if (!is_expression_boolean(apm, stmt->condition))
            {
                Expression *condition = get_expression(apm->expression, stmt->condition);
                raise_compilation_error(c, CONDITION_IS_NOT_BOOLEAN, condition->span);
            }
            break;
        }

        // Check initial value of variable declaration is correct type
        case VARIABLE_DECLARATION:
        {
            if (!stmt->has_initial_value)
                continue;

            RhinoType var_type = get_variable(apm->variable, stmt->variable)->type;
            RhinoType value_type = get_expression_type(apm, stmt->initial_value);

            if (!allow_assign_a_to_b(value_type, var_type))
                raise_compilation_error(c, RHS_TYPE_DOES_NOT_MATCH_LHS, stmt->span);

            break;
        }

        // Check rhs of assignment is a type that can be assigned to the lhs
        case ASSIGNMENT_STATEMENT:
        {
            RhinoType lhs_type = get_expression_type(apm, stmt->assignment_lhs);
            RhinoType rhs_type = get_expression_type(apm, stmt->assignment_rhs);

            if (!allow_assign_a_to_b(rhs_type, lhs_type))
                raise_compilation_error(c, RHS_TYPE_DOES_NOT_MATCH_LHS, stmt->span);

            break;
        }
        }
    }
}
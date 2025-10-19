#include "check.h"
#include "fatal_error.h"
#include <string.h>

void check_block(Compiler *c, Program *apm, Block *block);
void check_function(Compiler *c, Program *apm, Function *funct);
void check_statement_list(Compiler *c, Program *apm, StatementList statement_list);

void check_block(Compiler *c, Program *apm, Block *block)
{
    check_statement_list(c, apm, block->statements);
}

void check_function(Compiler *c, Program *apm, Function *funct)
{
    check_block(c, apm, funct->body);
}

void check_statement_list(Compiler *c, Program *apm, StatementList statement_list)
{
    Statement *stmt;
    StatementIterator it = statement_iterator(statement_list);
    while (stmt = next_statement_iterator(&it))
    {
        switch (stmt->kind)
        {

            // Recursively check nested blocks
        case CODE_BLOCK:
        {
            check_block(c, apm, stmt->block);
            break;
        }

        // Recursively check functions
        case FUNCTION_DECLARATION:
        {
            check_function(c, apm, stmt->function);
            break;
        }

        // Check if statement conditions are booleans
        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
        {
            RhinoSort condition_sort = get_expression_type(apm, c->source_text, stmt->condition).sort;
            if (condition_sort != SORT_BOOL && condition_sort != ERROR_SORT)
            {
                Expression *condition = stmt->condition;
                raise_compilation_error(c, CONDITION_IS_NOT_BOOLEAN, condition->span);
            }
            break;
        }

        // Check initial value of variable declaration is correct type
        case VARIABLE_DECLARATION:
        {
            if (!stmt->has_initial_value)
                continue;

            RhinoType var_type = stmt->variable->type;
            RhinoType value_type = get_expression_type(apm, c->source_text, stmt->initial_value);

            if (!allow_assign_a_to_b(value_type, var_type))
                raise_compilation_error(c, RHS_TYPE_DOES_NOT_MATCH_LHS, stmt->span);

            break;
        }

        // Check rhs of assignment is a type that can be assigned to the lhs
        case ASSIGNMENT_STATEMENT:
        {
            RhinoType lhs_type = get_expression_type(apm, c->source_text, stmt->assignment_lhs);
            RhinoType rhs_type = get_expression_type(apm, c->source_text, stmt->assignment_rhs);

            if (!allow_assign_a_to_b(rhs_type, lhs_type))
                raise_compilation_error(c, RHS_TYPE_DOES_NOT_MATCH_LHS, stmt->span);

            break;
        }
        }
    }
}

// CHECK //

void check(Compiler *c, Program *apm)
{
    check_block(c, apm, apm->program_block);

    // Produce errors for remaining identity literals
    // TODO: Reimplement this as part of the tree walk
    // {
    //     for (size_t i = 0; i < apm->expression.count; i++)
    //     {
    //         Expression *identity_literal = get_expression(apm->expression, i);
    //         if (identity_literal->kind == IDENTITY_LITERAL && !identity_literal->given_error)
    //         {
    //             raise_compilation_error(c, IDENTITY_DOES_NOT_EXIST, identity_literal->span);
    //             identity_literal->given_error = true;
    //         }
    //     }
    // }
}
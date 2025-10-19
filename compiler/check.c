#include "check.h"
#include "fatal_error.h"
#include <string.h>

void check_block(Compiler *c, Program *apm, Block *block);
void check_function(Compiler *c, Program *apm, Function *funct);
void check_statement_list(Compiler *c, Program *apm, StatementList statement_list);
void check_expression(Compiler *c, Program *apm, Expression *expr);

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

        case INVALID_STATEMENT:
            break;

        case VARIABLE_DECLARATION:
        {
            if (!stmt->has_initial_value)
                continue;

            check_expression(c, apm, stmt->initial_value);

            // Check initial value of variable declaration matches the variable's type
            RhinoType var_type = stmt->variable->type;
            RhinoType value_type = get_expression_type(apm, c->source_text, stmt->initial_value);

            if (!allow_assign_a_to_b(value_type, var_type))
                raise_compilation_error(c, RHS_TYPE_DOES_NOT_MATCH_LHS, stmt->span);

            break;
        }

        case FUNCTION_DECLARATION:
        {
            check_function(c, apm, stmt->function);
            break;
        }

        case ENUM_TYPE_DECLARATION:
            break;

        case STRUCT_TYPE_DECLARATION:
            // TODO: How to correctly check structs declarations?
            break;

        case CODE_BLOCK:
        {
            check_block(c, apm, stmt->block);
            break;
        }

        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
        {
            check_expression(c, apm, stmt->condition);
            check_block(c, apm, stmt->body);

            // Check if statement conditions are booleans
            RhinoSort condition_sort = get_expression_type(apm, c->source_text, stmt->condition).sort;
            if (condition_sort != SORT_BOOL && condition_sort != ERROR_SORT)
            {
                Expression *condition = stmt->condition;
                raise_compilation_error(c, CONDITION_IS_NOT_BOOLEAN, condition->span);
            }
            break;
        }

        case ELSE_SEGMENT:
            check_block(c, apm, stmt->body);
            break;

        case BREAK_LOOP:
            check_block(c, apm, stmt->body);
            break;

        case FOR_LOOP:
            check_expression(c, apm, stmt->iterable);
            check_block(c, apm, stmt->body);
            break;

        case BREAK_STATEMENT:
            break;

        case ASSIGNMENT_STATEMENT:
        {
            check_expression(c, apm, stmt->assignment_lhs);
            check_expression(c, apm, stmt->assignment_rhs);

            // Check rhs of assignment is a type that can be assigned to the lhs
            RhinoType lhs_type = get_expression_type(apm, c->source_text, stmt->assignment_lhs);
            RhinoType rhs_type = get_expression_type(apm, c->source_text, stmt->assignment_rhs);

            if (!allow_assign_a_to_b(rhs_type, lhs_type))
                raise_compilation_error(c, RHS_TYPE_DOES_NOT_MATCH_LHS, stmt->span);

            break;
        }

        case OUTPUT_STATEMENT:
        case EXPRESSION_STMT:
        case RETURN_STATEMENT:
            check_expression(c, apm, stmt->expression);
            break;

        default:
            fatal_error("Could not check %s statement", statement_kind_string(stmt->kind));
            break;
        }
    }
}

void check_expression(Compiler *c, Program *apm, Expression *expr)
{
    switch (expr->kind)
    {
    case INVALID_EXPRESSION:
        break;

    case IDENTITY_LITERAL:
    {
        if (!expr->given_error)
        {
            raise_compilation_error(c, IDENTITY_DOES_NOT_EXIST, expr->span);
            expr->given_error = true;
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
        check_expression(c, apm, expr->callee);

        Argument *arg;
        ArgumentIterator it = argument_iterator(expr->arguments);
        while (arg = next_argument_iterator(&it))
            check_expression(c, apm, arg->expr);

        break;
    }

    case INDEX_BY_FIELD:
        check_expression(c, apm, expr->subject);
        break;

    case RANGE_LITERAL:
        check_expression(c, apm, expr->first);
        check_expression(c, apm, expr->last);
        break;

    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_NOT:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        check_expression(c, apm, expr->operand);
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
        check_expression(c, apm, expr->lhs);
        check_expression(c, apm, expr->rhs);
        break;

    default:
        fatal_error("Could not check %s expression", expression_kind_string(expr->kind));
        break;
    }
}

// CHECK //

void check(Compiler *c, Program *apm)
{
    check_block(c, apm, apm->program_block);
}
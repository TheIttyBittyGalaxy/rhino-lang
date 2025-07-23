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

void resolve_types(Compiler *c, Program *apm)
{
    for (size_t i = 0; i < apm->statement.count; i++)
    {
        Statement *stmt = get_statement(apm->statement, i);

        if (stmt->kind == VARIABLE_DECLARATION)
        {
            Variable *var = get_variable(apm->variable, stmt->variable);

            if (stmt->has_type_expression)
            {
                Expression *type_expression = get_expression(apm->expression, stmt->type_expression);
                if (type_expression->kind != IDENTITY_LITERAL)
                {
                    // FIXME: Really the error here should be something like "invalid type expression"
                    raise_compilation_error(c, VARIABLE_DECLARED_WITH_INVALID_TYPE, type_expression->span);
                }

                type_expression->identity_resolved = true;

                if (substr_is(c->source_text, type_expression->identity, "int"))
                    var->type.sort = SORT_INT;
                else if (substr_is(c->source_text, type_expression->identity, "num"))
                    var->type.sort = SORT_NUM;
                else if (substr_is(c->source_text, type_expression->identity, "str"))
                    var->type.sort = SORT_STR;
                else
                {
                    bool enum_type_found = false;
                    for (size_t i = 0; i < apm->enum_type.count; i++)
                    {
                        EnumType *enum_type = get_enum_type(apm->enum_type, i);
                        if (substr_match(c->source_text, type_expression->identity, enum_type->identity))
                        {
                            var->type.sort = SORT_ENUM;
                            var->type.index = i;
                            enum_type_found = true;
                            break;
                        }
                    }

                    if (!enum_type_found)
                    {
                        var->type.sort = INVALID_SORT;
                        raise_compilation_error(c, VARIABLE_DECLARED_WITH_INVALID_TYPE, type_expression->span);
                    }
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

        // Skip identity literals that have been resolved in a previous pass
        if (identity_literal->identity_resolved)
            continue;

        // Enum types
        bool found_enum_type = false;
        for (size_t i = 0; i < apm->enum_type.count; i++)
        {
            EnumType *enum_type = get_enum_type(apm->enum_type, i);
            if (substr_match(c->source_text, identity_literal->identity, enum_type->identity))
            {
                identity_literal->kind = TYPE_REFERENCE;
                identity_literal->type.sort = SORT_ENUM;
                identity_literal->type.index = i;

                found_enum_type = true;
                break;
            }
        }

        if (found_enum_type)
            continue;

        // Could not resolve literal
        raise_compilation_error(c, VARIABLE_OR_ENUM_DOES_NOT_EXIST, identity_literal->span);
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

// ANALYSE //

void analyse(Compiler *c, Program *apm)
{
    determine_main_function(c, apm);

    resolve_types(c, apm);
    resolve_function_calls(c, apm);
    resolve_identity_literals(c, apm);
    resolve_enum_values(c, apm);

    check_conditions_are_booleans(c, apm);
    check_variable_assignments(c, apm);
}
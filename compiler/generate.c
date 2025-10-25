#include "fatal_error.h"
#include "generate.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

// TRANSPILER //

typedef struct
{
    FILE *output;
    const char *source_text;

    bool newline_pending;
    size_t indent;
} Generator;

// FORWARD DECLARATIONS //

void generate_type(Generator *t, Program *apm, RhinoType ty);
void generate_expression(Generator *t, Program *apm, Expression *expr);
void generate_statement(Generator *t, Program *apm, Statement *stmt);
void generate_block(Generator *t, Program *apm, Block *block);
void generate_function(Generator *t, Program *apm, Function *funct);
void generate_program(Generator *t, Program *apm);

// EMIT //

// TODO: Improve these implementations so that the output is formatted nicely

void emit(Generator *t, const char *str, ...)
{
    if (t->newline_pending)
    {
        fprintf(t->output, "\n");
        for (size_t i = 0; i < t->indent; i++)
            fprintf(t->output, "\t");
        t->newline_pending = false;
    }

    va_list args;
    va_start(args, str);
    vfprintf(t->output, str, args);
    va_end(args);
}

// NOTE: Will escape any "%" with "%%" - allows us to emit printf format strings
void emit_escaped(Generator *t, const char *str)
{
    // TODO: Check if C already implements this?
    // TODO: Check that this is escaping all the characters that it should
    // TODO: Avoid buffer overflow

    char buffer[1024];
    size_t i = 0; // buffer index
    size_t j = 0; // str index
    while (str[j] != '\0')
    {
        if (str[j] == '%')
        {
            buffer[i++] = '%';
            buffer[i++] = '%';
            j++;
        }
        else
        {
            buffer[i++] = str[j++];
        }
    }
    buffer[i] = '\0';

    emit(t, buffer);
}

void emit_substr(Generator *t, substr sub)
{
    emit(t, "%.*s", sub.len, t->source_text + sub.pos);
}

// TODO: Prevent multiple consecutive blank lines
void emit_line(Generator *t, const char *str, ...)
{
    va_list args;
    va_start(args, str);
    emit(t, str, args);
    va_end(args);

    t->newline_pending = true;
}

void emit_open_brace(Generator *t)
{
    t->newline_pending = true;
    emit_line(t, "{");
    t->indent++;
}

void emit_close_brace(Generator *t)
{
    t->newline_pending = true;
    t->indent--;
    emit_line(t, "}");
}

#define EMIT(...) emit(t, __VA_ARGS__)
#define EMIT_ESCAPED(str) emit_escaped(t, str)
#define EMIT_SUBSTR(sub) emit_substr(t, sub)
#define EMIT_LINE(...) emit_line(t, __VA_ARGS__)
#define EMIT_NEWLINE(str) emit_line(t, "")
#define EMIT_OPEN_BRACE() emit_open_brace(t)
#define EMIT_CLOSE_BRACE() emit_close_brace(t)

// TRANSPILE //

void generate_type(Generator *t, Program *apm, RhinoType ty)
{
    switch (ty.tag)
    {
    case RHINO_NATIVE_TYPE:
        if (IS_NONE_TYPE(ty))
            EMIT("void");
        else if (IS_BOOL_TYPE(ty))
            EMIT("bool");
        else if (IS_INT_TYPE(ty))
            EMIT("int");
        else if (IS_NUM_TYPE(ty))
            EMIT("double");
        else if (IS_STR_TYPE(ty))
            EMIT("char*");
        else
            fatal_error("Unable to generate native Rhino type %s.", ty.native_type->name);

        break;

    case RHINO_ENUM_TYPE:
        EMIT_SUBSTR(ty.enum_type->identity);
        break;

    case RHINO_STRUCT_TYPE:
        EMIT_SUBSTR(ty.struct_type->identity);
        break;

    default:
        fatal_error("Unable to generate Rhino type %s.", rhino_type_string(apm, ty));
    }
}

void generate_default_value(Generator *t, Program *apm, RhinoType ty)
{
    switch (ty.tag)
    {
    case RHINO_NATIVE_TYPE:
        if (IS_BOOL_TYPE(ty))
            EMIT("false");
        else if (IS_INT_TYPE(ty))
            EMIT("0");
        else if (IS_NUM_TYPE(ty))
            EMIT("0");
        else if (IS_STR_TYPE(ty))
            EMIT("\"\"");

        break;

    // FIXME: This is a bodge job just to get things working for now.
    //        The language semantics prohibit a default value for enums.
    //        (Though, none is the default of noneable enums).
    case RHINO_ENUM_TYPE:
        EMIT("(");
        EMIT_SUBSTR(ty.enum_type->identity);
        EMIT(")0");
        break;

    case RHINO_STRUCT_TYPE:
    {
        StructType *struct_type = ty.struct_type;

        EMIT("(");
        EMIT_SUBSTR(struct_type->identity);
        EMIT("){ ");

        Property *property;
        PropertyIterator it = property_iterator(struct_type->properties);
        size_t i = 0;
        while (property = next_property_iterator(&it))
        {
            if (i > 0)
                EMIT(", ");

            generate_default_value(t, apm, property->type);
            i++;
        }

        EMIT(" }");
        break;
    }

    default:
        fatal_error("Could not generate default value for value of type %s.", rhino_type_string(apm, ty));
    }
}

void generate_expression(Generator *t, Program *apm, Expression *expr)
{
    switch (expr->kind)
    {
    case IDENTITY_LITERAL:
        fatal_error("Attempt to generate an IDENTITY_LITERAL expression. By this pass we would have expected this to become a VARIABLE_REFERENCE.");
        break;

    case BOOLEAN_LITERAL:
        EMIT(expr->bool_value ? "true" : "false");
        break;

    case INTEGER_LITERAL:
        EMIT("%d", expr->integer_value);
        break;

    case FLOAT_LITERAL:
        EMIT("%f", expr->float_value);
        break;

    case STRING_LITERAL:
        EMIT("\"");
        EMIT_SUBSTR(expr->string_value);
        EMIT("\"");
        break;

    case ENUM_VALUE_LITERAL:
    {
        EnumValue *enum_value = expr->enum_value;
        EnumType *enum_type = enum_value->type_of_enum_value;

        EMIT_SUBSTR(enum_type->identity);
        EMIT("__");
        EMIT_SUBSTR(enum_value->identity);
        break;
    }

    case VARIABLE_REFERENCE:
    {
        Variable *var = expr->variable;
        EMIT_SUBSTR(var->identity);
        break;
    }

    case PARAMETER_REFERENCE:
    {
        EMIT_SUBSTR(expr->parameter->identity);
        break;
    }

    case FUNCTION_CALL:
    {
        Expression *reference = expr->callee;
        Function *callee = reference->function;
        EMIT_SUBSTR(callee->identity);
        EMIT("(");

        Argument *arg;
        ArgumentIterator it = argument_iterator(expr->arguments);
        size_t i = 0;
        while (arg = next_argument_iterator(&it))
        {
            if (i > 0)
                EMIT(",");

            generate_expression(t, apm, arg->expr);
            i++;
        }

        EMIT(")");
        break;
    }

    case INDEX_BY_FIELD:
    {
        generate_expression(t, apm, expr->subject);
        EMIT(".");
        EMIT_SUBSTR(expr->field);
        break;
    }

    case UNARY_POS:
    {
        generate_expression(t, apm, expr->operand);
        break;
    }

    case UNARY_NEG:
    {
        EMIT("-");
        generate_expression(t, apm, expr->operand);
        break;
    }

    case UNARY_NOT:
    {
        EMIT("!");
        generate_expression(t, apm, expr->operand);
        break;
    }

    case UNARY_INCREMENT:
    {
        generate_expression(t, apm, expr->operand);
        EMIT("++");
        break;
    }

    case UNARY_DECREMENT:
    {
        generate_expression(t, apm, expr->operand);
        EMIT("--");
        break;
    }

    // FIXME: Rhino and C may treat precedence differently. Ensure we insert extra ()s where required
    //        so that the C code produces the correct Rhino semantics.
#define CASE_BINARY(expr_kind, symbol)          \
    case expr_kind:                             \
    {                                           \
        generate_expression(t, apm, expr->lhs); \
        EMIT(" " symbol " ");                   \
        generate_expression(t, apm, expr->rhs); \
        break;                                  \
    }

        CASE_BINARY(BINARY_MULTIPLY, "*")
        CASE_BINARY(BINARY_DIVIDE, "/")
        CASE_BINARY(BINARY_REMAINDER, "%%")
        CASE_BINARY(BINARY_ADD, "+")
        CASE_BINARY(BINARY_SUBTRACT, "-")
        CASE_BINARY(BINARY_LESS_THAN, "<")
        CASE_BINARY(BINARY_GREATER_THAN, ">")
        CASE_BINARY(BINARY_LESS_THAN_EQUAL, "<=")
        CASE_BINARY(BINARY_GREATER_THAN_EQUAL, ">=")
        CASE_BINARY(BINARY_EQUAL, "==")
        CASE_BINARY(BINARY_NOT_EQUAL, "!=")
        CASE_BINARY(BINARY_LOGICAL_AND, "&&")
        CASE_BINARY(BINARY_LOGICAL_OR, "||")

#undef CASE_BINARY

    default:
        fatal_error("Could not generate %s expression.", expression_kind_string(expr->kind));
        break;
    }
}

void generate_statement(Generator *t, Program *apm, Statement *stmt)
{
    switch (stmt->kind)
    {

    case FUNCTION_DECLARATION:
    case ENUM_TYPE_DECLARATION:
    case STRUCT_TYPE_DECLARATION:
        break;

    case CODE_BLOCK:
    {
        generate_block(t, apm, stmt->block);
        break;
    }

    case ELSE_IF_SEGMENT:
        EMIT("else ");
    case IF_SEGMENT:
        EMIT("if (");
        generate_expression(t, apm, stmt->condition);
        EMIT(")");
        generate_block(t, apm, stmt->body);
        break;

    case ELSE_SEGMENT:
    {
        EMIT("else ");
        generate_block(t, apm, stmt->body);
        break;
    }

    case BREAK_LOOP:
    {
        EMIT_LINE("while (true)");
        generate_block(t, apm, stmt->body);
        break;
    }

    case FOR_LOOP:
    {
        Variable *iterator = stmt->iterator;
        Expression *iterable = stmt->iterable;

        if (iterable->kind == RANGE_LITERAL)
        {
            EMIT("for (int ");
            EMIT_SUBSTR(iterator->identity);
            EMIT(" = ");
            generate_expression(t, apm, iterable->first);
            EMIT("; ");

            EMIT_SUBSTR(iterator->identity);
            EMIT(" <= ");
            generate_expression(t, apm, iterable->last);
            EMIT("; ++", iterable->last);

            EMIT_SUBSTR(iterator->identity);
            EMIT(")");

            generate_block(t, apm, stmt->body);
            break;
        }

        if (iterable->kind == TYPE_REFERENCE && iterable->type.tag == RHINO_ENUM_TYPE)
        {
            EnumType *enum_type = iterable->type.enum_type;

            EMIT("for (");
            EMIT_SUBSTR(enum_type->identity);
            EMIT(" ");
            EMIT_SUBSTR(iterator->identity);
            EMIT(" = (");
            EMIT_SUBSTR(enum_type->identity);
            EMIT(")0; ");

            EMIT_SUBSTR(iterator->identity);
            EMIT(" < %d; ", enum_type->values.count);

            EMIT_SUBSTR(iterator->identity);
            EMIT(" = (");
            EMIT_SUBSTR(enum_type->identity);
            EMIT(")(");
            EMIT_SUBSTR(iterator->identity);
            EMIT(" + 1))");

            generate_block(t, apm, stmt->body);
            break;
        }

        fatal_error("Could not generate for loop with %s iterable.", expression_kind_string(iterable->kind));
        break;
    }

    case WHILE_LOOP:
    {
        EMIT("while (");
        generate_expression(t, apm, stmt->condition);
        EMIT_LINE(")");
        generate_block(t, apm, stmt->body);
        break;
    }

    case BREAK_STATEMENT:
    {
        EMIT_LINE("break;");
        break;
    }

    case ASSIGNMENT_STATEMENT:
    {
        generate_expression(t, apm, stmt->assignment_lhs);
        EMIT(" = ");
        generate_expression(t, apm, stmt->assignment_rhs);

        EMIT_LINE(";");
        break;
    }

    case VARIABLE_DECLARATION:
    {
        Variable *var = stmt->variable;
        generate_type(t, apm, var->type);
        EMIT(" ");
        EMIT_SUBSTR(var->identity);
        EMIT(" = ");

        if (stmt->initial_value)
            generate_expression(t, apm, stmt->initial_value);
        else
            generate_default_value(t, apm, var->type);

        EMIT_LINE(";");
        break;
    }

    case OUTPUT_STATEMENT:
    {
        Expression *expr = stmt->expression;
        RhinoType expr_type = get_expression_type(apm, t->source_text, expr);

        switch (expr_type.tag)
        {
        case RHINO_NATIVE_TYPE:
            if (IS_BOOL_TYPE(expr_type))
            {
                EMIT_ESCAPED("printf(\"%s\\n\", (");
                generate_expression(t, apm, expr);
                EMIT_LINE(") ? \"true\" : \"false\");");
                break;
            }

            if (IS_STR_TYPE(expr_type))
            {
                if (expr->kind == STRING_LITERAL)
                {
                    EMIT("printf(\"");
                    EMIT_SUBSTR(expr->string_value);
                    EMIT_LINE("\\n\");");
                }
                else
                {
                    EMIT_ESCAPED("printf(\"%s\\n\", ");
                    generate_expression(t, apm, expr);
                    EMIT_LINE(");");
                }
                break;
            }

            if (IS_INT_TYPE(expr_type))
            {
                EMIT_ESCAPED("printf(\"%d\\n\", ");
                generate_expression(t, apm, expr);
                EMIT_LINE(");");
                break;
            }

            if (IS_NUM_TYPE(expr_type))
            {
                EMIT_OPEN_BRACE();

                EMIT("float_to_str(");
                generate_expression(t, apm, expr);
                EMIT_LINE(");");

                EMIT_ESCAPED("printf(\"%s\\n\", __to_str_buffer);");

                EMIT_CLOSE_BRACE();
                break;
            }

            fatal_error("Unable to generate output statement for expression with native type %s.", expr_type.native_type->name);
            break;

        case RHINO_ENUM_TYPE:
            EMIT_ESCAPED("printf(\"%s\\n\", string_of_");
            EMIT_SUBSTR(expr_type.enum_type->identity);
            EMIT("(");
            generate_expression(t, apm, expr);
            EMIT_LINE("));");
            break;

        default:
            fatal_error("Unable to generate output statement for expression with type %s.", rhino_type_string(apm, expr_type));
        }

        break;
    }

    case EXPRESSION_STMT:
    {
        generate_expression(t, apm, stmt->expression);
        EMIT_LINE(";");
        break;
    }

    case RETURN_STATEMENT:
    {
        EMIT("return ");
        generate_expression(t, apm, stmt->expression);
        EMIT_LINE(";");
        break;
    }

    default:
        fatal_error("Could not generate %s statement.", statement_kind_string(stmt->kind));
        break;
    }
}

void generate_block(Generator *t, Program *apm, Block *block)
{
    EMIT_OPEN_BRACE();

    Statement *child;
    StatementIterator it = statement_iterator(block->statements);
    while (child = next_statement_iterator(&it))
        generate_statement(t, apm, child);

    EMIT_CLOSE_BRACE();
}

void generate_function_signature(Generator *t, Program *apm, Function *funct)
{
    generate_type(t, apm, funct->return_type);
    EMIT(" ");
    EMIT_SUBSTR(funct->identity);
    EMIT("(");

    Parameter *parameter;
    ParameterIterator it = parameter_iterator(funct->parameters);
    size_t i = 0;
    while (parameter = next_parameter_iterator(&it))
    {
        if (i > 0)
            EMIT(", ");

        generate_type(t, apm, parameter->type);
        EMIT(" ");
        EMIT_SUBSTR(parameter->identity);
        i++;
    }

    EMIT(")");
}

void generate_function(Generator *t, Program *apm, Function *funct)
{
    generate_function_signature(t, apm, funct);
    generate_block(t, apm, funct->body);
}

void generate_program(Generator *t, Program *apm)
{
    // Rhino Runtime
    // NOTE: rhino/runtime_as_str.c should be a C string containing rhino/runtime.c.
    //       We are currently generating this with a build script
    fprintf(t->output,
#include "rhino/runtime_as_str.c"
    );

    Statement *declaration;
    StatementIterator it;

    // Global enum types
    it = statement_iterator(apm->program_block->statements);
    while (declaration = next_statement_iterator(&it))
    {
        if (declaration->kind == ENUM_TYPE_DECLARATION)
        {
            EnumType *enum_type = declaration->enum_type;
            EnumValue *enum_value;
            EnumValueIterator it;
            size_t i = 0;

            // Enum declaration
            EMIT("typedef enum { ");

            it = enum_value_iterator(enum_type->values);
            i = 0;
            while (enum_value = next_enum_value_iterator(&it))
            {
                if (i > 0)
                    EMIT(", ");

                EMIT_SUBSTR(enum_type->identity);
                EMIT("__");
                EMIT_SUBSTR(enum_value->identity);

                i++;
            }

            EMIT(" } ");
            EMIT_SUBSTR(enum_type->identity);
            EMIT_LINE(";");
            EMIT_NEWLINE();

            // To string function
            EMIT("const char* string_of_");
            EMIT_SUBSTR(enum_type->identity);
            EMIT("(");
            EMIT_SUBSTR(enum_type->identity);
            EMIT(" value)");
            EMIT_NEWLINE();

            EMIT_OPEN_BRACE();
            EMIT_LINE("switch(value)");
            EMIT_OPEN_BRACE();

            it = enum_value_iterator(enum_type->values);
            i = 0;
            while (enum_value = next_enum_value_iterator(&it))
            {
                EMIT("case ");
                EMIT_SUBSTR(enum_type->identity);
                EMIT("__");
                EMIT_SUBSTR(enum_value->identity);

                EMIT(": return \"");
                EMIT_SUBSTR(enum_value->identity);
                EMIT_LINE("\";");
            }

            EMIT_CLOSE_BRACE();
            EMIT_CLOSE_BRACE();
            EMIT_NEWLINE();
        }
    }
    EMIT_NEWLINE();

    // Global struct types
    it = statement_iterator(apm->program_block->statements);
    while (declaration = next_statement_iterator(&it))
    {
        if (declaration->kind == STRUCT_TYPE_DECLARATION)
        {
            StructType *struct_type = declaration->struct_type;

            // Sstruct declaration
            EMIT_LINE("typedef struct");
            EMIT_OPEN_BRACE();

            Property *property;
            PropertyIterator it = property_iterator(struct_type->properties);
            size_t i = 0;
            while (property = next_property_iterator(&it))
            {
                generate_type(t, apm, property->type);
                EMIT(" ");
                EMIT_SUBSTR(property->identity);
                EMIT_LINE(";");
            }

            EMIT_CLOSE_BRACE();
            EMIT_SUBSTR(struct_type->identity);
            EMIT_LINE(";");
            EMIT_NEWLINE();

            // To string function
            EMIT("const char* string_of_");
            EMIT_SUBSTR(struct_type->identity);
            EMIT("(");
            EMIT_SUBSTR(struct_type->identity);
            EMIT(" value)");
            EMIT_NEWLINE();

            // TODO: Serialise the struct in some helpful way
            EMIT_OPEN_BRACE();
            EMIT("return \"");
            EMIT_SUBSTR(struct_type->identity);
            EMIT("\";");
            EMIT_NEWLINE();
            EMIT_CLOSE_BRACE();
            EMIT_NEWLINE();
        }
    }
    EMIT_NEWLINE();

    // Declare global variables
    it = statement_iterator(apm->program_block->statements);
    while (declaration = next_statement_iterator(&it))
    {
        if (declaration->kind == VARIABLE_DECLARATION)
        {
            Variable *var = declaration->variable;
            generate_type(t, apm, var->type);
            EMIT(" ");
            EMIT_SUBSTR(var->identity);

            if (declaration->initial_value && declaration->variable->order < 2)
            {
                EMIT(" = ");
                generate_expression(t, apm, declaration->initial_value);
            }
            else if (!declaration->initial_value)
            {
                EMIT(" = ");
                generate_default_value(t, apm, var->type);
            }

            EMIT_LINE(";");
        }
    }

    // Forward declarations for global functoins
    it = statement_iterator(apm->program_block->statements);
    while (declaration = next_statement_iterator(&it))
    {
        if (declaration->kind == FUNCTION_DECLARATION && declaration->function != apm->main)
        {
            generate_function_signature(t, apm, declaration->function);
            EMIT_LINE(";");
        }
    }
    EMIT_NEWLINE();

    // Function definitions for global functoins
    it = statement_iterator(apm->program_block->statements);
    while (declaration = next_statement_iterator(&it))
    {
        if (declaration->kind == FUNCTION_DECLARATION && declaration->function != apm->main)
            generate_function(t, apm, declaration->function);
    }
    EMIT_NEWLINE();

    // Main - Function
    EMIT_LINE("int main(int argc, char *argv[])");
    EMIT_OPEN_BRACE();

    // Main - Initialise global variables
    size_t order = 2;
    bool is_last_order = false;
    while (!is_last_order)
    {
        is_last_order = true;
        it = statement_iterator(apm->program_block->statements);
        while (declaration = next_statement_iterator(&it))
        {
            if (declaration->kind == VARIABLE_DECLARATION && declaration->variable->order == order)
            {
                assert(declaration->initial_value);
                is_last_order = false;

                EMIT_SUBSTR(declaration->variable->identity);
                EMIT(" = ");
                generate_expression(t, apm, declaration->initial_value);
                EMIT_LINE(";");
            }
        }
        order++;
    }

    // Main - User main function
    generate_block(t, apm, apm->main->body);

    EMIT_CLOSE_BRACE();
}

void generate(Compiler *compiler, Program *apm)
{
    FILE *handle = fopen("_out.c", "w");
    if (handle == NULL)
    {
        fatal_error("Could not open _out.c for writing");
        return;
    }

    Generator generator;
    generator.output = handle;
    generator.source_text = compiler->source_text;

    generator.newline_pending = false;
    generator.indent = 0;

    generate_program(&generator, apm);

    fclose(handle);
}
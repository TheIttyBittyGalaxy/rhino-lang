#include "fatal_error.h"
#include "transpile.h"
#include <stdarg.h>
#include <stdio.h>

// TRANSPILER //

typedef struct
{
    FILE *output;
    const char *source_text;

    bool newline_pending;
    size_t indent;
} Transpiler;

// FORWARD DECLARATIONS //

void transpile_type(Transpiler *t, Program *apm, RhinoType rhino_type);
void transpile_expression(Transpiler *t, Program *apm, size_t expr_index);
void transpile_statement(Transpiler *t, Program *apm, size_t stmt_index);
void transpile_function(Transpiler *t, Program *apm, size_t funct_index);
void transpile_program(Transpiler *t, Program *apm);

// EMIT //

// TODO: Improve these implementations so that the output is formatted nicely

void emit(Transpiler *t, const char *str, ...)
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
void emit_escaped(Transpiler *t, const char *str)
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

void emit_substr(Transpiler *t, substr sub)
{
    emit(t, "%.*s", sub.len, t->source_text + sub.pos);
}

// TODO: Prevent multiple consecutive blank lines
void emit_line(Transpiler *t, const char *str, ...)
{
    va_list args;
    va_start(args, str);
    emit(t, str, args);
    va_end(args);

    t->newline_pending = true;
}

void emit_open_brace(Transpiler *t)
{
    t->newline_pending = true;
    emit_line(t, "{");
    t->indent++;
}

void emit_close_brace(Transpiler *t)
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

void transpile_type(Transpiler *t, Program *apm, RhinoType rhino_type)
{
    switch (rhino_type.sort)
    {
    case SORT_BOOL:
        EMIT("bool");
        break;

    case SORT_STR:
        EMIT("char*");
        break;

    case SORT_INT:
        EMIT("int");
        break;

    case SORT_NUM:
        EMIT("double");
        break;

    case SORT_ENUM:
    {
        EnumType *enum_type = get_enum_type(apm->enum_type, rhino_type.index);
        EMIT_SUBSTR(enum_type->identity);
        break;
    }

    default:
        fatal_error("Unable to generate Rhino type %s.", rhino_type_string(apm, rhino_type));
    }
}

void transpile_expression(Transpiler *t, Program *apm, size_t expr_index)
{
    Expression *expr = get_expression(apm->expression, expr_index);

    switch (expr->kind)
    {
    case IDENTITY_LITERAL:
        fatal_error("Attempt to transpile an IDENTITY_LITERAL expression. By this pass we would have expected this to become a VARIABLE_REFERENCE.");
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
        EnumValue *enum_value = get_enum_value(apm->enum_value, expr->enum_value);
        size_t enum_type_index = get_enum_type_of_enum_value(apm, expr->enum_value);
        EnumType *enum_type = get_enum_type(apm->enum_type, enum_type_index);

        EMIT_SUBSTR(enum_type->identity);
        EMIT("__");
        EMIT_SUBSTR(enum_value->identity);
        break;
    }

    case VARIABLE_REFERENCE:
    {
        Variable *var = get_variable(apm->variable, expr->variable);
        EMIT_SUBSTR(var->identity);
        break;
    }

    case FUNCTION_CALL:
    {
        Function *callee = get_function(apm->function, expr->callee);
        EMIT_SUBSTR(callee->identity);
        EMIT("()");
        break;
    }

    case UNARY_POS:
    {
        transpile_expression(t, apm, expr->operand);
        break;
    }

    case UNARY_NEG:
    {
        EMIT("-");
        transpile_expression(t, apm, expr->operand);
        break;
    }

    // FIXME: Rhino and C may treat precedence differently. Ensure we insert extra ()s where required
    //        so that the C code produces the correct Rhino semantics.
#define CASE_BINARY(expr_kind, symbol)           \
    case expr_kind:                              \
    {                                            \
        transpile_expression(t, apm, expr->lhs); \
        EMIT(" " symbol " ");                    \
        transpile_expression(t, apm, expr->rhs); \
        break;                                   \
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
        fatal_error("Could not transpile %s expression.", expression_kind_string(expr->kind));
        break;
    }
}

void transpile_statement(Transpiler *t, Program *apm, size_t stmt_index)
{
    Statement *stmt = get_statement(apm->statement, stmt_index);

    switch (stmt->kind)
    {
    case CODE_BLOCK:
    case SINGLE_BLOCK:
    {
        EMIT_OPEN_BRACE();

        // TODO: At the moment we are walking the APM as a tree. However, it is probably possible
        //       and also faster to just walk it as a list, where we create a "{" whenever we
        //       encounter a code block, and use a stack to track where we should insert the "}".
        size_t n = get_first_statement_in_code_block(apm, stmt);
        while (n < stmt->statements.count)
        {
            transpile_statement(t, apm, stmt->statements.first + n);
            n = get_next_statement_in_code_block(apm, stmt, n);
        }

        EMIT_CLOSE_BRACE();
        break;
    }

    case ELSE_IF_SEGMENT:
        EMIT("else ");
    case IF_SEGMENT:
        EMIT("if (");
        transpile_expression(t, apm, stmt->condition);
        EMIT(")");
        transpile_statement(t, apm, stmt->body);
        break;

    case ELSE_SEGMENT:
    {
        EMIT("else ");
        transpile_statement(t, apm, stmt->body);
        break;
    }

    case BREAK_LOOP:
    {
        EMIT_LINE("while (true)");
        transpile_statement(t, apm, stmt->body);
        break;
    }

    case FOR_LOOP:
    {
        Variable *iterator = get_variable(apm->variable, stmt->iterator);
        Expression *iterable = get_expression(apm->expression, stmt->iterable);

        if (iterable->kind == RANGE_LITERAL)
        {
            EMIT("for (int ");
            EMIT_SUBSTR(iterator->identity);
            EMIT(" = ");
            transpile_expression(t, apm, iterable->first);
            EMIT("; ");

            EMIT_SUBSTR(iterator->identity);
            EMIT(" <= ");
            transpile_expression(t, apm, iterable->last);
            EMIT("; ++", iterable->last);

            EMIT_SUBSTR(iterator->identity);
            EMIT(")");

            transpile_statement(t, apm, stmt->body);
            break;
        }

        if (iterable->kind == TYPE_REFERENCE && iterable->type.sort == SORT_ENUM)
        {
            EnumType *enum_type = get_enum_type(apm->enum_type, iterable->type.index);

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

            transpile_statement(t, apm, stmt->body);
            break;
        }

        fatal_error("Could not transpile for loop with %s iterable.", expression_kind_string(iterable->kind));
        break;
    }

    case BREAK_STATEMENT:
    {
        EMIT_LINE("break;");
        break;
    }

    case ASSIGNMENT_STATEMENT:
    {
        transpile_expression(t, apm, stmt->assignment_lhs);
        EMIT(" = ");
        transpile_expression(t, apm, stmt->assignment_rhs);

        EMIT_LINE(";");
        break;
    }

    case VARIABLE_DECLARATION:
    {
        Variable *var = get_variable(apm->variable, stmt->variable);
        transpile_type(t, apm, var->type);
        EMIT(" ");
        EMIT_SUBSTR(var->identity);
        EMIT(" = ");

        if (stmt->has_initial_value)
            transpile_expression(t, apm, stmt->initial_value);
        else if (var->type.sort == SORT_BOOL)
            EMIT("false");
        else if (var->type.sort == SORT_STR)
            EMIT("\"\"");
        else if (var->type.sort == SORT_INT)
            EMIT("0");
        else if (var->type.sort == SORT_NUM)
            EMIT("0");

        EMIT_LINE(";");
        break;
    }

    case OUTPUT_STATEMENT:
    {
        size_t expr_index = stmt->expression;
        RhinoType expr_type = get_expression_type(apm, expr_index);

        switch (expr_type.sort)
        {

        case SORT_BOOL:
        {
            EMIT_ESCAPED("printf(\"%s\\n\", (");
            transpile_expression(t, apm, expr_index);
            EMIT_LINE(") ? \"true\" : \"false\");");
            break;
        }

        case SORT_STR:
        {
            Expression *expr = get_expression(apm->expression, expr_index);

            if (expr->kind == STRING_LITERAL)
            {
                EMIT("printf(\"");
                EMIT_SUBSTR(expr->string_value);
                EMIT_LINE("\\n\");");
            }
            else
            {
                EMIT_ESCAPED("printf(\"%s\\n\", ");
                transpile_expression(t, apm, expr_index);
                EMIT_LINE(");");
            }
            break;
        }

        case SORT_INT:
        {
            EMIT_ESCAPED("printf(\"%d\\n\", ");
            transpile_expression(t, apm, expr_index);
            EMIT_LINE(");");
            break;
        }

        case SORT_NUM:
        {
            EMIT_OPEN_BRACE();

            EMIT("float_to_str(");
            transpile_expression(t, apm, expr_index);
            EMIT_LINE(");");

            EMIT_ESCAPED("printf(\"%s\\n\", __to_str_buffer);");

            EMIT_CLOSE_BRACE();
            break;
        }

        case SORT_ENUM:
        {
            EnumType *enum_type = get_enum_type(apm->enum_type, expr_type.index);

            EMIT_ESCAPED("printf(\"%s\\n\", string_of_");
            EMIT_SUBSTR(enum_type->identity);
            EMIT("(");
            transpile_expression(t, apm, expr_index);
            EMIT_LINE("));");
            break;
        }

        default:
            fatal_error("Unable to generate output statement for expression with type %s.", rhino_type_string(apm, expr_type));
        }

        break;
    }

    case EXPRESSION_STMT:
    {
        transpile_expression(t, apm, stmt->expression);
        EMIT_LINE(";");
        break;
    }

    case FUNCTION_DECLARATION:
        break;

    default:
        fatal_error("Could not transpile %s statement.", statement_kind_string(stmt->kind));
        break;
    }
}

void transpile_function_signature(Transpiler *t, Program *apm, size_t funct_index)
{
    Function *funct = get_function(apm->function, funct_index);

    EMIT("void ");
    EMIT_SUBSTR(funct->identity);
    EMIT("()");
}

void transpile_function(Transpiler *t, Program *apm, size_t funct_index)
{
    Function *funct = get_function(apm->function, funct_index);

    transpile_function_signature(t, apm, funct_index);
    transpile_statement(t, apm, funct->body);
}

void transpile_program(Transpiler *t, Program *apm)
{
    // Rhino Runtime
    // NOTE: rhino/runtime_as_str.c should be a C string containing rhino/runtime.c.
    //       We are currently generating this with a build script
    fprintf(t->output,
#include "rhino/runtime_as_str.c"
    );

    // Enum types
    for (size_t i = 0; i < apm->enum_type.count; i++)
    {
        EnumType *enum_type = get_enum_type(apm->enum_type, i);

        // Enum declaration
        EMIT("typedef enum { ");
        size_t last = enum_type->values.first + enum_type->values.count - 1;
        for (size_t i = enum_type->values.first; i <= last; i++)
        {
            EnumValue *enum_value = get_enum_value(apm->enum_value, i);
            EMIT_SUBSTR(enum_type->identity);
            EMIT("__");
            EMIT_SUBSTR(enum_value->identity);
            if (i < last)
                EMIT(", ");
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
        for (size_t i = enum_type->values.first; i <= last; i++)
        {
            EnumValue *enum_value = get_enum_value(apm->enum_value, i);
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
    EMIT_NEWLINE();

    // Forward function declarations
    for (size_t i = 0; i < apm->function.count; i++)
    {
        if (i == apm->main)
            continue;
        transpile_function_signature(t, apm, i);
        EMIT_LINE(";");
    }
    EMIT_NEWLINE();

    // Function definitions
    for (size_t i = 0; i < apm->function.count; i++)
    {
        if (i == apm->main)
            continue;
        transpile_function(t, apm, i);
        EMIT_NEWLINE();
    }

    // Main
    Function *main_funct = get_function(apm->function, apm->main);

    EMIT_LINE("int main(int argc, char *argv[])");
    EMIT_OPEN_BRACE();
    transpile_statement(t, apm, main_funct->body);
    EMIT_CLOSE_BRACE();
}

void transpile(Compiler *compiler, Program *apm)
{
    FILE *handle = fopen("_out.c", "w");
    if (handle == NULL)
    {
        fatal_error("Could not open _out.c for writing");
        return;
    }

    Transpiler transpiler;
    transpiler.output = handle;
    transpiler.source_text = compiler->source_text;

    transpiler.newline_pending = false;
    transpiler.indent = 0;

    transpile_program(&transpiler, apm);

    fclose(handle);
}
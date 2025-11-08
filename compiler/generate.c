#include "fatal_error.h"
#include "generate.h"
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

// GENERATOR //

typedef struct
{
    FILE *output;
    const char *source_text;

    bool newline_pending;
    size_t indent;
} Generator;

// FORWARD DECLARATIONS //

void generate_type(Generator *g, CProgram *cpm, CType ty);
void generate_var_declaration(Generator *g, CProgram *cpm, CVarDeclaration decl);
void generate_expression(Generator *g, CProgram *cpm, CExpression *expr);
void generate_statement(Generator *g, CProgram *cpm, CStatement *stmt);
void generate_block(Generator *g, CProgram *cpm, CBlock *block);
void generate_function(Generator *g, CProgram *cpm, CFunction *funct);
void generate_program(Generator *g, CProgram *cpm);

// EMIT //

// TODO: Improve these implementations so that the output is formatted nicely

void emit(Generator *g, const char *str, ...)
{
    if (g->newline_pending)
    {
        fprintf(g->output, "\n");
        for (size_t i = 0; i < g->indent; i++)
            fprintf(g->output, "\t");
        g->newline_pending = false;
    }

    va_list args;
    va_start(args, str);
    vfprintf(g->output, str, args);
    va_end(args);
}

// NOTE: Will escape any "%" with "%%" - allows us to emit printf format strings
void emit_escaped(Generator *g, const char *str)
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

    emit(g, buffer);
}

void emit_substr(Generator *g, substr sub)
{
    emit(g, "%.*s", sub.len, g->source_text + sub.pos);
}

void emit_c_identity(Generator *g, CIdentity id)
{
    if (id.from_source)
        emit_substr(g, id.sub);
    else
        emit(g, id.str);
}

// TODO: Prevent multiple consecutive blank lines
void emit_line(Generator *g, const char *str, ...)
{
    va_list args;
    va_start(args, str);
    emit(g, str, args);
    va_end(args);

    g->newline_pending = true;
}

void emit_open_brace(Generator *g)
{
    g->newline_pending = true;
    emit_line(g, "{");
    g->indent++;
}

void emit_close_brace(Generator *g)
{
    g->newline_pending = true;
    g->indent--;
    emit_line(g, "}");
}

#define EMIT(...) emit(g, __VA_ARGS__)
#define EMIT_ESCAPED(str) emit_escaped(g, str)
#define EMIT_SUBSTR(sub) emit_substr(g, sub)
#define EMIT_C_IDENTITY(id) emit_c_identity(g, id)
#define EMIT_LINE(...) emit_line(g, __VA_ARGS__)
#define EMIT_NEWLINE(str) emit_line(g, "")
#define EMIT_OPEN_BRACE() emit_open_brace(g)
#define EMIT_CLOSE_BRACE() emit_close_brace(g)

// TRANSPILE //

void generate_type(Generator *g, CProgram *cpm, CType ty)
{
    if (ty.is_const)
        EMIT("const ");

    switch (ty.tag)
    {
    case C_VOID:
        EMIT("void");
        break;

    case C_BOOL:
        EMIT("bool");
        break;

    case C_INT:
        EMIT("int");
        break;

    case C_FLOAT:
        EMIT("float");
        break;

    case C_STRING:
        EMIT("char*");
        break;

    case C_ENUM_TYPE:
        EMIT_SUBSTR(ty.enum_type->identity);
        break;

        // case C_STRUCT_TYPE:
        //     EMIT_SUBSTR(ty.enum_type->identity);
        //     break;

    default:
        fatal_error("Unable to generate C type %s.", c_type_tag_string(ty.tag));
    }
}

// void generate_default_value(Generator *g, CProgram *cpm, RhinoType ty)
// {
//     switch (ty.tag)
//     {
//     case RHINO_NATIVE_TYPE:
//         if (IS_BOOL_TYPE(ty))
//             EMIT("false");
//         else if (IS_INT_TYPE(ty))
//             EMIT("0");
//         else if (IS_NUM_TYPE(ty))
//             EMIT("0");
//         else if (IS_STR_TYPE(ty))
//             EMIT("\"\"");

//         break;

//     // FIXME: This is a bodge job just to get things working for now.
//     //        The language semantics prohibit a default value for enums.
//     //        (Though, none is the default of noneable enums).
//     case RHINO_ENUM_TYPE:
//         EMIT("(");
//         EMIT_SUBSTR(ty.enum_type->identity);
//         EMIT(")0");
//         break;

//     case RHINO_STRUCT_TYPE:
//     {
//         StructType *struct_type = ty.struct_type;

//         EMIT("(");
//         EMIT_SUBSTR(struct_type->identity);
//         EMIT("){ ");

//         Property *property;
//         PropertyIterator it = property_iterator(struct_type->properties);
//         size_t i = 0;
//         while (property = next_property_iterator(&it))
//         {
//             if (i > 0)
//                 EMIT(", ");

//             generate_default_value(g, cpm, property->type);
//             i++;
//         }

//         EMIT(" }");
//         break;
//     }

//     default:
//         fatal_error("Could not generate default value for value of type %s.", rhino_type_string(cpm, ty));
//     }
// }

void generate_var_declaration(Generator *g, CProgram *cpm, CVarDeclaration decl)
{
    CVariable *var = decl.variable;

    generate_type(g, cpm, var->type);
    EMIT(" ");
    EMIT_SUBSTR(var->identity);

    if (decl.initial_value)
    {
        EMIT(" = ");
        generate_expression(g, cpm, decl.initial_value);
    }
}

void generate_expression(Generator *g, CProgram *cpm, CExpression *expr)
{
    switch (expr->kind)
    {
    case C_BOOLEAN_LITERAL:
        EMIT(expr->bool_value ? "true" : "false");
        break;

    case C_INTEGER_LITERAL:
        EMIT("%d", expr->integer_value);
        break;

    case C_FLOAT_LITERAL:
        EMIT("%f", expr->float_value);
        break;

    case C_STRING_LITERAL:
        EMIT("\"%s\"", expr->string_value);
        break;

    case C_SOURCE_STRING_LITERAL:
        EMIT("\"");
        EMIT_SUBSTR(expr->source_string);
        EMIT("\"");
        break;

        // case ENUM_VALUE_LITERAL:
        // {
        //     EnumValue *enum_value = expr->enum_value;
        //     EnumType *enum_type = enum_value->type_of_enum_value;

        //     EMIT_SUBSTR(enum_type->identity);
        //     EMIT("__");
        //     EMIT_SUBSTR(enum_value->identity);
        //     break;
        // }

    case C_VARIABLE_REFERENCE:
        EMIT_SUBSTR(expr->variable->identity);
        break;

    case C_TYPE_REFERENCE:
        generate_type(g, cpm, expr->type);
        break;

        // case PARAMETER_REFERENCE:
        // {
        //     EMIT_SUBSTR(expr->parameter->identity);
        //     break;
        // }

    case C_FUNCTION_CALL:
    {
        // TODO: I'm not sure yet if this will always be true?
        assert(expr->callee->kind == C_FUNCTION_REFERENCE);

        CExpression *reference = expr->callee;
        CFunction *callee = reference->function;
        EMIT_C_IDENTITY(callee->identity);
        EMIT("(");

        CArgument *arg;
        CArgumentIterator it = c_argument_iterator(expr->arguments);
        size_t i = 0;
        while (arg = next_c_argument_iterator(&it))
        {
            if (i > 0)
                EMIT(",");

            generate_expression(g, cpm, arg->expr);
            i++;
        }

        EMIT(")");
        break;
    }

        // case INDEX_BY_FIELD:
        // {
        //     generate_expression(g, cpm, expr->subject);
        //     EMIT(".");
        //     EMIT_SUBSTR(expr->field);
        //     break;
        // }

    case C_UNARY_POS:
        generate_expression(g, cpm, expr->operand);
        break;

    case C_UNARY_NEG:
        EMIT("-");
        generate_expression(g, cpm, expr->operand);
        break;

    case C_UNARY_NOT:
        EMIT("!");
        generate_expression(g, cpm, expr->operand);
        break;

    case C_UNARY_PREFIX_INCREMENT:
        EMIT("++");
        generate_expression(g, cpm, expr->operand);
        break;

    case C_UNARY_PREFIX_DECREMENT:
        EMIT("--");
        generate_expression(g, cpm, expr->operand);
        break;

    case C_UNARY_POSTFIX_INCREMENT:
        generate_expression(g, cpm, expr->operand);
        EMIT("++");
        break;

    case C_UNARY_POSTFIX_DECREMENT:
        generate_expression(g, cpm, expr->operand);
        EMIT("--");
        break;

    case C_TYPE_CAST:
        EMIT("(");
        generate_expression(g, cpm, expr->lhs);
        EMIT(")");
        generate_expression(g, cpm, expr->rhs);
        break;

        // FIXME: Rhino and C may treat precedence differently. Ensure we insert extra ()s where required
        //        so that the C code produces the correct Rhino semantics.
#define CASE_BINARY(expr_kind, symbol)          \
    case expr_kind:                             \
    {                                           \
        generate_expression(g, cpm, expr->lhs); \
        EMIT(" " symbol " ");                   \
        generate_expression(g, cpm, expr->rhs); \
        break;                                  \
    }

        CASE_BINARY(C_ASSIGNMENT, "=")
        CASE_BINARY(C_BINARY_MULTIPLY, "*")
        CASE_BINARY(C_BINARY_DIVIDE, "/")
        CASE_BINARY(C_BINARY_REMAINDER, "%%")
        CASE_BINARY(C_BINARY_ADD, "+")
        CASE_BINARY(C_BINARY_SUBTRACT, "-")
        CASE_BINARY(C_BINARY_LESS_THAN, "<")
        CASE_BINARY(C_BINARY_GREATER_THAN, ">")
        CASE_BINARY(C_BINARY_LESS_THAN_EQUAL, "<=")
        CASE_BINARY(C_BINARY_GREATER_THAN_EQUAL, ">=")
        CASE_BINARY(C_BINARY_EQUAL, "==")
        CASE_BINARY(C_BINARY_NOT_EQUAL, "!=")
        CASE_BINARY(C_BINARY_LOGICAL_AND, "&&")
        CASE_BINARY(C_BINARY_LOGICAL_OR, "||")

#undef CASE_BINARY

    default:
        fatal_error("Could not generate %s expression.", c_expression_kind_string(expr->kind));
        break;
    }
}

void generate_statement(Generator *g, CProgram *cpm, CStatement *stmt)
{
    switch (stmt->kind)
    {
        // case C_CODE_BLOCK:
        //     generate_block(g, cpm, stmt->block);
        //     break;

    case C_ELSE_IF_SEGMENT:
        EMIT("else ");
    case C_IF_SEGMENT:
        EMIT("if (");
        generate_expression(g, cpm, stmt->condition);
        EMIT(")");
        generate_block(g, cpm, stmt->body);
        break;

    case C_ELSE_SEGMENT:
        EMIT("else ");
        generate_block(g, cpm, stmt->body);
        break;

        // case BREAK_LOOP:
        // {
        //     EMIT_LINE("while (true)");
        //     generate_block(g, cpm, stmt->body);
        //     break;
        // }

    case C_FOR_LOOP:
        EMIT("for (");
        generate_var_declaration(g, cpm, stmt->init);
        EMIT("; ");
        generate_expression(g, cpm, stmt->condition);
        EMIT("; ");
        generate_expression(g, cpm, stmt->advance);
        EMIT(")");

        generate_block(g, cpm, stmt->body);
        break;

        // case WHILE_LOOP:
        // {
        //     EMIT("while (");
        //     generate_expression(g, cpm, stmt->condition);
        //     EMIT_LINE(")");
        //     generate_block(g, cpm, stmt->body);
        //     break;
        // }

        // case BREAK_STATEMENT:
        // {
        //     EMIT_LINE("break;");
        //     break;
        // }

        // case ASSIGNMENT_STATEMENT:
        // {
        //     generate_expression(g, cpm, stmt->assignment_lhs);
        //     EMIT(" = ");
        //     generate_expression(g, cpm, stmt->assignment_rhs);

        //     EMIT_LINE(";");
        //     break;
        // }

    case C_VARIABLE_DECLARATION:
        generate_var_declaration(g, cpm, stmt->var_declaration);
        EMIT_LINE(";");
        break;

    case C_EXPRESSION_STMT:
        generate_expression(g, cpm, stmt->expr);
        EMIT_LINE(";");
        break;

        // case RETURN_STATEMENT:
        // {
        //     EMIT("return ");
        //     generate_expression(g, cpm, stmt->expression);
        //     EMIT_LINE(";");
        //     break;
        // }

    default:
        fatal_error("Could not generate %s statement.", c_statement_kind_string(stmt->kind));
        break;
    }
}

void generate_block(Generator *g, CProgram *cpm, CBlock *block)
{
    EMIT_OPEN_BRACE();

    CStatement *stmt;
    CStatementIterator it = c_statement_iterator(block->statements);
    while (stmt = next_c_statement_iterator(&it))
        generate_statement(g, cpm, stmt);

    EMIT_CLOSE_BRACE();
}

void generate_function_signature(Generator *g, CProgram *cpm, CFunction *funct)
{
    generate_type(g, cpm, funct->return_type);
    EMIT(" ");
    EMIT_C_IDENTITY(funct->identity);
    EMIT("(");

    // FIXME: Implement
    // Parameter *parameter;
    // ParameterIterator it = parameter_iterator(funct->parameters);
    // size_t i = 0;
    // while (parameter = next_parameter_iterator(&it))
    // {
    //     if (i > 0)
    //         EMIT(", ");

    //     generate_type(g, cpm, parameter->type);
    //     EMIT(" ");
    //     EMIT_SUBSTR(parameter->identity);
    //     i++;
    // }

    EMIT(")");
}

void generate_function(Generator *g, CProgram *cpm, CFunction *funct)
{
    generate_function_signature(g, cpm, funct);
    generate_block(g, cpm, funct->body);
}

void generate_program(Generator *g, CProgram *cpm)
{
    // Rhino Runtime
    // NOTE: rhino/runtime_as_str.c should be a C string containing rhino/runtime.c.
    //       We are currently generating this with a build script
    fprintf(g->output,
#include "rhino/runtime_as_str.c"
    );

    // Statement *declaration;
    // StatementIterator it;

    // Enum types
    CEnumType *enum_type;
    CEnumTypeIterator enum_type_it = c_enum_type_iterator(get_c_enum_type_list(cpm->enum_type));
    while (enum_type = next_c_enum_type_iterator(&enum_type_it))
    {
        EMIT("typedef enum { ");

        for (size_t i = 0; i < enum_type->count; i++)
        {
            CEnumValue *enum_value = get_c_enum_value(get_c_enum_value_list(cpm->enum_value), enum_type->first + i);
            if (i > 0)
                EMIT(", ");

            EMIT_SUBSTR(enum_value->identity);
        }

        EMIT(" } ");
        EMIT_SUBSTR(enum_type->identity);
        EMIT_LINE(";");
        EMIT_NEWLINE();
    }

    // Structs
    // it = statement_iterator(cpm->program_block->statements);
    // while (declaration = next_statement_iterator(&it))
    // {
    //     if (declaration->kind == STRUCT_TYPE_DECLARATION)
    //     {
    //         StructType *struct_type = declaration->struct_type;

    //         // Sstruct declaration
    //         EMIT_LINE("typedef struct");
    //         EMIT_OPEN_BRACE();

    //         Property *property;
    //         PropertyIterator it = property_iterator(struct_type->properties);
    //         size_t i = 0;
    //         while (property = next_property_iterator(&it))
    //         {
    //             generate_type(g, cpm, property->type);
    //             EMIT(" ");
    //             EMIT_SUBSTR(property->identity);
    //             EMIT_LINE(";");
    //         }

    //         EMIT_CLOSE_BRACE();
    //         EMIT_SUBSTR(struct_type->identity);
    //         EMIT_LINE(";");
    //         EMIT_NEWLINE();

    //         // To string function
    //         EMIT("const char* string_of_");
    //         EMIT_SUBSTR(struct_type->identity);
    //         EMIT("(");
    //         EMIT_SUBSTR(struct_type->identity);
    //         EMIT(" value)");
    //         EMIT_NEWLINE();

    //         // TODO: Serialise the struct in some helpful way
    //         EMIT_OPEN_BRACE();
    //         EMIT("return \"");
    //         EMIT_SUBSTR(struct_type->identity);
    //         EMIT("\";");
    //         EMIT_NEWLINE();
    //         EMIT_CLOSE_BRACE();
    //         EMIT_NEWLINE();
    //     }
    // }
    // EMIT_NEWLINE();

    // Declare global variables
    // it = statement_iterator(cpm->program_block->statements);
    // while (declaration = next_statement_iterator(&it))
    // {
    //     if (declaration->kind == VARIABLE_DECLARATION)
    //     {
    //         Variable *var = declaration->variable;
    //         generate_type(g, cpm, var->type);
    //         EMIT(" ");
    //         EMIT_SUBSTR(var->identity);

    //         if (declaration->initial_value && declaration->variable->order < 2)
    //         {
    //             EMIT(" = ");
    //             generate_expression(g, cpm, declaration->initial_value);
    //         }
    //         else if (!declaration->initial_value)
    //         {
    //             EMIT(" = ");
    //             generate_default_value(g, cpm, var->type);
    //         }

    //         EMIT_LINE(";");
    //     }
    // }

    // Function declarations
    CFunction *funct;
    CFunctionIterator funct_it = c_function_iterator(get_c_function_list(cpm->function));
    while (funct = next_c_function_iterator(&funct_it))
    {
        generate_function_signature(g, cpm, funct);
        EMIT_LINE(";");
    }
    EMIT_NEWLINE();

    // Function definitions for global functoins
    funct_it = c_function_iterator(get_c_function_list(cpm->function));
    while (funct = next_c_function_iterator(&funct_it))
    {
        generate_function(g, cpm, funct);
    }
    EMIT_NEWLINE();

    // // Main - Function
    // EMIT_LINE("int main(int argc, char *argv[])");
    // EMIT_OPEN_BRACE();

    // // Main - Initialise global variables
    // size_t order = 2;
    // bool is_last_order = false;
    // while (!is_last_order)
    // {
    //     is_last_order = true;
    //     it = statement_iterator(cpm->program_block->statements);
    //     while (declaration = next_statement_iterator(&it))
    //     {
    //         if (declaration->kind == VARIABLE_DECLARATION && declaration->variable->order == order)
    //         {
    //             assert(declaration->initial_value);
    //             is_last_order = false;

    //             EMIT_SUBSTR(declaration->variable->identity);
    //             EMIT(" = ");
    //             generate_expression(g, cpm, declaration->initial_value);
    //             EMIT_LINE(";");
    //         }
    //     }
    //     order++;
    // }

    // // Main - User main function
    // generate_block(g, cpm, cpm->main->body);

    // EMIT_CLOSE_BRACE();
}

void generate(Compiler *compiler, CProgram *cpm)
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

    generate_program(&generator, cpm);

    fclose(handle);
}
#include "transpile.h"

#include "fatal_error.h"

// PAGES //

// TODO: Alter this code to use a bucket allocator?

#define PAGE_SIZE 65536

typedef struct Book Book;
typedef struct Page Page;

char book_write_buffer[65536];

void init_book(Book *book);
Page *create_page();
void write(Book *book, const char *str, ...);
void vwrite(Book *book, const char *str, va_list args);
void write_book_to_file(Book *book, FILE *file);

struct Book
{
    Page *first;
    Page *current;
    bool newline_pending;
    size_t indent;
};

struct Page
{
    Page *next;
    size_t length;
    char content[PAGE_SIZE];
};

void init_book(Book *book)
{
    book->first = create_page();
    book->current = book->first;
    book->newline_pending = false;
    book->indent = 0;
}

Page *create_page()
{
    Page *page = (Page *)malloc(sizeof(Page));

    page->next = NULL;
    page->length = 0;
    page->content[0] = '\0';

    return page;
}

void vwrite(Book *book, const char *str, va_list args)
{
    // FIXME: Do we need to be able to handle a situation where the string being written is larger than the size of a page?

    int chars_used = vsprintf(book_write_buffer, str, args);
    assert(chars_used >= 0); // If chars_used is negative, vsprintf is indicating that an error has occurred

    assert(chars_used + 1 < PAGE_SIZE); // Ensure the string, including the null character, can fit in a single page

    if (chars_used == 0)
        return;

    Page *page = book->current;
    if (page->length + chars_used + 1 > PAGE_SIZE)
    {
        Page *next = create_page();
        page->next = next;
        book->current = next;
        page = next;
    }

    memmove(page->content + page->length, book_write_buffer, chars_used + 1);
    page->length += chars_used;
}

void write(Book *book, const char *str, ...)
{
    va_list args;
    va_start(args, str);
    vwrite(book, str, args);
    va_end(args);
}

void write_book_to_file(Book *book, FILE *file)
{
    Page *page = book->first;
    while (page != NULL)
    {
        fputs(page->content, file);
        page = page->next;
    }
}

// TRANSPILER //

typedef struct
{
    const char *source_text;

    Book body;

    // FIXME: Make this dynamically allocated/resizable to avoid buffer overflow?
    char identity_buffer[512];
} Transpiler;

// GET IDENTITY //

const char *get_c_identity(Transpiler *t, void *apm_node, substr identity)
{
    sprintf(t->identity_buffer, "%.*s__%p", identity.len, t->source_text + identity.pos, apm_node);
    return &t->identity_buffer[0];
}

#define GET_C_IDENTITY(ptr) get_c_identity(t, (void *)ptr, ptr->identity)

// EMIT //

// TODO: Improve these implementations so that the output is formatted nicely

void emit(Book *book, const char *str, ...)
{
    if (book->newline_pending)
    {
        write(book, "\n");
        for (size_t i = 0; i < book->indent; i++)
            write(book, "\t");
        book->newline_pending = false;
    }

    va_list args;
    va_start(args, str);
    vwrite(book, str, args);
    va_end(args);
}

void emit_escaped(Book *book, const char *str)
{
    emit(book, "%s", str);
}

void emit_substr(Book *book, const char *source_text, substr sub)
{
    emit(book, "%.*s", sub.len, source_text + sub.pos);
}

// TODO: Prevent multiple consecutive blank lines
void emit_line(Book *book, const char *str, ...)
{
    va_list args;
    va_start(args, str);
    emit(book, str, args);
    va_end(args);

    book->newline_pending = true;
}

void emit_open_brace(Book *book)
{
    book->newline_pending = true;
    emit_line(book, "{");
    book->indent++;
}

void emit_close_brace(Book *book)
{
    book->newline_pending = true;
    book->indent--;
    emit_line(book, "}");
}

#define EMIT(...) emit(b, __VA_ARGS__)
#define EMIT_ESCAPED(str) emit_escaped(b, str)
#define EMIT_SUBSTR(sub) emit_substr(b, t->source_text, sub)
#define EMIT_LINE(...) emit_line(b, __VA_ARGS__)
#define EMIT_NEWLINE(str) emit_line(b, "")
#define EMIT_OPEN_BRACE() emit_open_brace(b)
#define EMIT_CLOSE_BRACE() emit_close_brace(b)

// TRANSPILE //

void transpile_type(Transpiler *t, Book *b, Program *apm, RhinoType ty);
void transpile_default_value(Transpiler *t, Book *b, Program *apm, RhinoType ty);
void transpile_expression(Transpiler *t, Book *b, Program *apm, Expression *expr);
void transpile_statement(Transpiler *t, Book *b, Program *apm, Statement *stmt);
void transpile_block(Transpiler *t, Book *b, Program *apm, Block *block);
void transpile_function_signature(Transpiler *t, Book *b, Program *apm, Function *funct);
void transpile_function(Transpiler *t, Book *b, Program *apm, Function *funct);
void transpile_program(Transpiler *t, Book *b, Program *apm);

void transpile_type(Transpiler *t, Book *b, Program *apm, RhinoType ty)
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
        EMIT(GET_C_IDENTITY(ty.enum_type));
        break;

    case RHINO_STRUCT_TYPE:
        EMIT(GET_C_IDENTITY(ty.struct_type));
        break;

    default:
        fatal_error("Unable to generate Rhino type %s.", rhino_type_string(apm, ty));
    }
}

void transpile_default_value(Transpiler *t, Book *b, Program *apm, RhinoType ty)
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
        EMIT(GET_C_IDENTITY(ty.enum_type));
        EMIT(")0");
        break;

    case RHINO_STRUCT_TYPE:
    {
        StructType *struct_type = ty.struct_type;

        EMIT("(");
        EMIT(GET_C_IDENTITY(struct_type));
        EMIT("){ ");

        Property *property;
        PropertyIterator it = property_iterator(struct_type->properties);
        size_t i = 0;
        while (property = next_property_iterator(&it))
        {
            if (i > 0)
                EMIT(", ");

            transpile_default_value(t, b, apm, property->type);
            i++;
        }

        EMIT(" }");
        break;
    }

    default:
        fatal_error("Could not transpile default value for value of type %s.", rhino_type_string(apm, ty));
    }
}

void transpile_expression(Transpiler *t, Book *b, Program *apm, Expression *expr)
{
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
        EnumValue *enum_value = expr->enum_value;
        EnumType *enum_type = enum_value->type_of_enum_value;
        EMIT(GET_C_IDENTITY(enum_value));
        break;
    }

    case VARIABLE_REFERENCE:
    {
        Variable *var = expr->variable;
        EMIT(GET_C_IDENTITY(var));
        break;
    }

    case PARAMETER_REFERENCE:
    {
        EMIT(GET_C_IDENTITY(expr->parameter));
        break;
    }

    case FUNCTION_CALL:
    {
        Expression *reference = expr->callee;
        Function *callee = reference->function;
        EMIT(GET_C_IDENTITY(callee));
        EMIT("(");

        Argument *arg;
        ArgumentIterator it = argument_iterator(expr->arguments);
        size_t i = 0;
        while (arg = next_argument_iterator(&it))
        {
            if (i > 0)
                EMIT(",");

            transpile_expression(t, b, apm, arg->expr);
            i++;
        }

        EMIT(")");
        break;
    }

    case INDEX_BY_FIELD:
    {
        transpile_expression(t, b, apm, expr->subject);
        EMIT(".");
        EMIT_SUBSTR(expr->field);
        break;
    }

    case UNARY_POS:
    {
        transpile_expression(t, b, apm, expr->operand);
        break;
    }

    case UNARY_NEG:
    {
        EMIT("-");
        transpile_expression(t, b, apm, expr->operand);
        break;
    }

    case UNARY_NOT:
    {
        EMIT("!");
        transpile_expression(t, b, apm, expr->operand);
        break;
    }

    case UNARY_INCREMENT:
    {
        transpile_expression(t, b, apm, expr->operand);
        EMIT("++");
        break;
    }

    case UNARY_DECREMENT:
    {
        transpile_expression(t, b, apm, expr->operand);
        EMIT("--");
        break;
    }

    // FIXME: Rhino and C may treat precedence differently. Ensure we insert extra ()s where required
    //        so that the C code produces the correct Rhino semantics.
#define CASE_BINARY(expr_kind, symbol)              \
    case expr_kind:                                 \
    {                                               \
        transpile_expression(t, b, apm, expr->lhs); \
        EMIT(" " symbol " ");                       \
        transpile_expression(t, b, apm, expr->rhs); \
        break;                                      \
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

void transpile_statement(Transpiler *t, Book *b, Program *apm, Statement *stmt)
{
    switch (stmt->kind)
    {

    case FUNCTION_DECLARATION:
    case ENUM_TYPE_DECLARATION:
    case STRUCT_TYPE_DECLARATION:
        break;

    case CODE_BLOCK:
    {
        transpile_block(t, b, apm, stmt->block);
        break;
    }

    case ELSE_IF_SEGMENT:
        EMIT("else ");
    case IF_SEGMENT:
        EMIT("if (");
        transpile_expression(t, b, apm, stmt->condition);
        EMIT(")");
        transpile_block(t, b, apm, stmt->body);
        break;

    case ELSE_SEGMENT:
    {
        EMIT("else ");
        transpile_block(t, b, apm, stmt->body);
        break;
    }

    case BREAK_LOOP:
    {
        EMIT_LINE("while (true)");
        transpile_block(t, b, apm, stmt->body);
        break;
    }

    case FOR_LOOP:
    {
        Variable *iterator = stmt->iterator;
        Expression *iterable = stmt->iterable;

        if (iterable->kind == RANGE_LITERAL)
        {
            EMIT("for (int ");
            EMIT(GET_C_IDENTITY(iterator));
            EMIT(" = ");
            transpile_expression(t, b, apm, iterable->first);
            EMIT("; ");

            EMIT(GET_C_IDENTITY(iterator));
            EMIT(" <= ");
            transpile_expression(t, b, apm, iterable->last);
            EMIT("; ++", iterable->last);

            EMIT(GET_C_IDENTITY(iterator));
            EMIT(")");

            transpile_block(t, b, apm, stmt->body);
            break;
        }

        if (iterable->kind == TYPE_REFERENCE && iterable->type.tag == RHINO_ENUM_TYPE)
        {
            EnumType *enum_type = iterable->type.enum_type;

            EMIT("for (");
            EMIT(GET_C_IDENTITY(enum_type));
            EMIT(" ");
            EMIT(GET_C_IDENTITY(iterator));
            EMIT(" = (");
            EMIT(GET_C_IDENTITY(enum_type));
            EMIT(")0; ");

            EMIT(GET_C_IDENTITY(iterator));
            EMIT(" < %d; ", enum_type->values.count);

            EMIT(GET_C_IDENTITY(iterator));
            EMIT(" = (");
            EMIT(GET_C_IDENTITY(enum_type));
            EMIT(")(");
            EMIT(GET_C_IDENTITY(iterator));
            EMIT(" + 1))");

            transpile_block(t, b, apm, stmt->body);
            break;
        }

        fatal_error("Could not transpile for loop with %s iterable.", expression_kind_string(iterable->kind));
        break;
    }

    case WHILE_LOOP:
    {
        EMIT("while (");
        transpile_expression(t, b, apm, stmt->condition);
        EMIT_LINE(")");
        transpile_block(t, b, apm, stmt->body);
        break;
    }

    case BREAK_STATEMENT:
    {
        EMIT_LINE("break;");
        break;
    }

    case ASSIGNMENT_STATEMENT:
    {
        transpile_expression(t, b, apm, stmt->assignment_lhs);
        EMIT(" = ");
        transpile_expression(t, b, apm, stmt->assignment_rhs);

        EMIT_LINE(";");
        break;
    }

    case VARIABLE_DECLARATION:
    {
        Variable *var = stmt->variable;
        transpile_type(t, b, apm, var->type);
        EMIT(" ");
        EMIT(GET_C_IDENTITY(var));
        EMIT(" = ");

        if (stmt->initial_value)
            transpile_expression(t, b, apm, stmt->initial_value);
        else
            transpile_default_value(t, b, apm, var->type);

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
                transpile_expression(t, b, apm, expr);
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
                    transpile_expression(t, b, apm, expr);
                    EMIT_LINE(");");
                }
                break;
            }

            if (IS_INT_TYPE(expr_type))
            {
                EMIT_ESCAPED("printf(\"%d\\n\", ");
                transpile_expression(t, b, apm, expr);
                EMIT_LINE(");");
                break;
            }

            if (IS_NUM_TYPE(expr_type))
            {
                EMIT_OPEN_BRACE();

                EMIT("float_to_str(");
                transpile_expression(t, b, apm, expr);
                EMIT_LINE(");");

                EMIT_ESCAPED("printf(\"%s\\n\", __to_str_buffer);");

                EMIT_CLOSE_BRACE();
                break;
            }

            fatal_error("Unable to generate output statement for expression with native type %s.", expr_type.native_type->name);
            break;

        case RHINO_ENUM_TYPE:
            EMIT_ESCAPED("printf(\"%s\\n\", string_of_");
            EMIT(GET_C_IDENTITY(expr_type.enum_type));
            EMIT("(");
            transpile_expression(t, b, apm, expr);
            EMIT_LINE("));");
            break;

        default:
            fatal_error("Unable to generate output statement for expression with type %s.", rhino_type_string(apm, expr_type));
        }

        break;
    }

    case EXPRESSION_STMT:
    {
        transpile_expression(t, b, apm, stmt->expression);
        EMIT_LINE(";");
        break;
    }

    case RETURN_STATEMENT:
    {
        EMIT("return ");
        transpile_expression(t, b, apm, stmt->expression);
        EMIT_LINE(";");
        break;
    }

    default:
        fatal_error("Could not transpile %s statement.", statement_kind_string(stmt->kind));
        break;
    }
}

void transpile_block(Transpiler *t, Book *b, Program *apm, Block *block)
{
    EMIT_OPEN_BRACE();

    Statement *child;
    StatementIterator it = statement_iterator(block->statements);
    while (child = next_statement_iterator(&it))
        transpile_statement(t, b, apm, child);

    EMIT_CLOSE_BRACE();
}

void transpile_function_signature(Transpiler *t, Book *b, Program *apm, Function *funct)
{
    transpile_type(t, b, apm, funct->return_type);
    EMIT(" ");
    EMIT(GET_C_IDENTITY(funct));
    EMIT("(");

    Parameter *parameter;
    ParameterIterator it = parameter_iterator(funct->parameters);
    size_t i = 0;
    while (parameter = next_parameter_iterator(&it))
    {
        if (i > 0)
            EMIT(", ");

        transpile_type(t, b, apm, parameter->type);
        EMIT(" ");
        EMIT(GET_C_IDENTITY(parameter));
        i++;
    }

    EMIT(")");
}

void transpile_function(Transpiler *t, Book *b, Program *apm, Function *funct)
{
    transpile_function_signature(t, b, apm, funct);
    transpile_block(t, b, apm, funct->body);
}

void transpile_program(Transpiler *t, Book *b, Program *apm)
{
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

                EMIT(GET_C_IDENTITY(enum_value));
                i++;
            }

            EMIT(" } ");
            EMIT(GET_C_IDENTITY(enum_type));
            EMIT_LINE(";");
            EMIT_NEWLINE();

            // To string function
            EMIT("const char* string_of_");
            EMIT(GET_C_IDENTITY(enum_type));
            EMIT("(");
            EMIT(GET_C_IDENTITY(enum_type));
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
                EMIT(GET_C_IDENTITY(enum_value));

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
                transpile_type(t, b, apm, property->type);
                EMIT(" ");
                EMIT_SUBSTR(property->identity);
                EMIT_LINE(";");
            }

            EMIT_CLOSE_BRACE();
            EMIT(GET_C_IDENTITY(struct_type));
            EMIT_LINE(";");
            EMIT_NEWLINE();

            // To string function
            EMIT("const char* string_of_");
            EMIT(GET_C_IDENTITY(struct_type));
            EMIT("(");
            EMIT(GET_C_IDENTITY(struct_type));
            EMIT(" value)");
            EMIT_NEWLINE();

            // TODO: Serialise the struct in some helpful way
            EMIT_OPEN_BRACE();
            EMIT("return \"");
            EMIT(GET_C_IDENTITY(struct_type));
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
            transpile_type(t, b, apm, var->type);
            EMIT(" ");
            EMIT(GET_C_IDENTITY(var));

            if (declaration->initial_value && declaration->variable->order < 2)
            {
                EMIT(" = ");
                transpile_expression(t, b, apm, declaration->initial_value);
            }
            else if (!declaration->initial_value)
            {
                EMIT(" = ");
                transpile_default_value(t, b, apm, var->type);
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
            transpile_function_signature(t, b, apm, declaration->function);
            EMIT_LINE(";");
        }
    }
    EMIT_NEWLINE();

    // Function definitions for global functoins
    it = statement_iterator(apm->program_block->statements);
    while (declaration = next_statement_iterator(&it))
    {
        if (declaration->kind == FUNCTION_DECLARATION && declaration->function != apm->main)
            transpile_function(t, b, apm, declaration->function);
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

                EMIT(GET_C_IDENTITY(declaration->variable));
                EMIT(" = ");
                transpile_expression(t, b, apm, declaration->initial_value);
                EMIT_LINE(";");
            }
        }
        order++;
    }

    // Main - User main function
    transpile_block(t, b, apm, apm->main->body);

    EMIT_CLOSE_BRACE();
}

void transpile(Compiler *compiler, Program *apm)
{
    Transpiler transpiler;
    transpiler.source_text = compiler->source_text;

    init_book(&transpiler.body);

    transpile_program(&transpiler, &transpiler.body, apm);

    FILE *output_file = fopen("_out.c", "w");
    if (output_file == NULL)
    {
        fatal_error("Could not open _out.c for writing");
        return;
    }

    // Rhino Runtime
    // NOTE: rhino/runtime_as_str.c should be a C string containing rhino/runtime.c.
    //       We are currently generating this with a build script
    fprintf(output_file,
#include "rhino/runtime_as_str.c"
    );

    write_book_to_file(&transpiler.body, output_file);

    fclose(output_file);
}
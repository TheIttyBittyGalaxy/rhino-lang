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

    if (book->newline_pending)
        fputs("\n", file);
}

// TRANSPILER //

typedef struct
{
    Program *apm;
    const char *source_text;

    Book enum_definitions;
    Book struct_declarations;
    Book struct_definitions;
    Book function_declarations;
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

void transpile_type(Transpiler *t, Book *b, RhinoType ty);
void transpile_default_value(Transpiler *t, Book *b, RhinoType ty);

void transpile_code_block(Transpiler *t, Book *b, Block *block);
void transpile_expression(Transpiler *t, Book *b, Expression *expr);
void transpile_function_signature(Transpiler *t, Book *b, Function *funct);

void transpile_declarations_in_block(Transpiler *t, Block *block);
void transpile_main_function(Transpiler *t);

void transpile_program(Transpiler *t);

// TRANSPILE TYPES //

void transpile_type(Transpiler *t, Book *b, RhinoType ty)
{
    Program *apm = t->apm;
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

void transpile_default_value(Transpiler *t, Book *b, RhinoType ty)
{
    Program *apm = t->apm;
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

            transpile_default_value(t, b, property->type);
            i++;
        }

        EMIT(" }");
        break;
    }

    default:
        fatal_error("Could not transpile default value for value of type %s.", rhino_type_string(apm, ty));
    }
}

// TRANSPILE CODE //

void transpile_code_block(Transpiler *t, Book *b, Block *block)
{
    assert(!block->declaration_block);

    EMIT_OPEN_BRACE();

    Statement *stmt;
    StatementIterator it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        switch (stmt->kind)
        {

        case FUNCTION_DECLARATION:
        case ENUM_TYPE_DECLARATION:
        case STRUCT_TYPE_DECLARATION:
            break;

        case CODE_BLOCK:
            transpile_code_block(t, b, stmt->block);
            break;

        case ELSE_IF_SEGMENT:
            EMIT("else ");
        case IF_SEGMENT:
            EMIT("if (");
            transpile_expression(t, b, stmt->condition);
            EMIT(")");
            transpile_code_block(t, b, stmt->body);
            break;

        case ELSE_SEGMENT:
            EMIT("else ");
            transpile_code_block(t, b, stmt->body);
            break;

        case BREAK_LOOP:
            EMIT_LINE("while (true)");
            transpile_code_block(t, b, stmt->body);
            break;

        case FOR_LOOP:
        {
            Variable *iterator = stmt->iterator;
            Expression *iterable = stmt->iterable;

            if (iterable->kind == RANGE_LITERAL)
            {
                EMIT("for (int ");
                EMIT(GET_C_IDENTITY(iterator));
                EMIT(" = ");
                transpile_expression(t, b, iterable->first);
                EMIT("; ");

                EMIT(GET_C_IDENTITY(iterator));
                EMIT(" <= ");
                transpile_expression(t, b, iterable->last);
                EMIT("; ++", iterable->last);

                EMIT(GET_C_IDENTITY(iterator));
                EMIT(")");

                transpile_code_block(t, b, stmt->body);
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

                transpile_code_block(t, b, stmt->body);
                break;
            }

            fatal_error("Could not transpile for loop with %s iterable.", expression_kind_string(iterable->kind));
            break;
        }

        case WHILE_LOOP:
            EMIT("while (");
            transpile_expression(t, b, stmt->condition);
            EMIT_LINE(")");
            transpile_code_block(t, b, stmt->body);
            break;

        case BREAK_STATEMENT:
            EMIT_LINE("break;");
            break;

        case ASSIGNMENT_STATEMENT:
            transpile_expression(t, b, stmt->assignment_lhs);
            EMIT(" = ");
            transpile_expression(t, b, stmt->assignment_rhs);
            EMIT_LINE(";");
            break;

        case VARIABLE_DECLARATION:
            transpile_type(t, b, stmt->variable->type);
            EMIT(" ");
            EMIT(GET_C_IDENTITY(stmt->variable));
            EMIT(" = ");

            if (stmt->initial_value)
                transpile_expression(t, b, stmt->initial_value);
            else
                transpile_default_value(t, b, stmt->variable->type);

            EMIT_LINE(";");
            break;

        case OUTPUT_STATEMENT:
        {
            Program *apm = t->apm;
            Expression *expr = stmt->expression;
            RhinoType expr_type = get_expression_type(t->apm, t->source_text, expr);

            switch (expr_type.tag)
            {
            case RHINO_NATIVE_TYPE:
                if (IS_BOOL_TYPE(expr_type))
                {
                    EMIT_ESCAPED("printf(\"%s\\n\", (");
                    transpile_expression(t, b, expr);
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
                        transpile_expression(t, b, expr);
                        EMIT_LINE(");");
                    }
                    break;
                }

                if (IS_INT_TYPE(expr_type))
                {
                    EMIT_ESCAPED("printf(\"%d\\n\", ");
                    transpile_expression(t, b, expr);
                    EMIT_LINE(");");
                    break;
                }

                if (IS_NUM_TYPE(expr_type))
                {
                    EMIT_OPEN_BRACE();

                    EMIT("float_to_str(");
                    transpile_expression(t, b, expr);
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
                transpile_expression(t, b, expr);
                EMIT_LINE("));");
                break;

            default:
                fatal_error("Unable to generate output statement for expression with type %s.", rhino_type_string(t->apm, expr_type));
            }

            break;
        }

        case EXPRESSION_STMT:
            transpile_expression(t, b, stmt->expression);
            EMIT_LINE(";");
            break;

        case RETURN_STATEMENT:
            EMIT("return ");
            transpile_expression(t, b, stmt->expression);
            EMIT_LINE(";");
            break;

        default:
            fatal_error("Could not transpile %s statement in code block.", statement_kind_string(stmt->kind));
            break;
        }
    }

    EMIT_CLOSE_BRACE();
}

// FIXME: Rhino precedence semantics may not match C precedence semantics
int c_precedence(ExpressionKind kind)
{
    return (int)precedence_of(kind);
}

void transpile_expression_with_caller_precedence(Transpiler *t, Book *b, Expression *expr, int caller_precedence)
{
    if (c_precedence(expr->kind) < caller_precedence)
        EMIT("(");

    switch (expr->kind)
    {
    case IDENTITY_LITERAL:
        fatal_error("Attempt to transpile an IDENTITY_LITERAL expression. By this pass we would have expected this to become a VARIABLE_REFERENCE.");
        break;

    case NONE_LITERAL:
        if (expr->none_variant == NONE_NULL)
            EMIT("NULL");
        else
            fatal_error("Could not transpile %s NONE_LITERAL variant.", none_variant_string(expr->none_variant));
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
        EMIT(GET_C_IDENTITY(expr->enum_value));
        break;

    case VARIABLE_REFERENCE:
        EMIT(GET_C_IDENTITY(expr->variable));
        break;

    case PARAMETER_REFERENCE:
        EMIT(GET_C_IDENTITY(expr->parameter));
        break;

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

            transpile_expression_with_caller_precedence(t, b, arg->expr, c_precedence(expr->kind));
            i++;
        }

        EMIT(")");
        break;
    }

    case INDEX_BY_FIELD:
        transpile_expression_with_caller_precedence(t, b, expr->subject, c_precedence(expr->kind));
        EMIT(".");
        EMIT_SUBSTR(expr->field);
        break;

    case UNARY_POS:
        transpile_expression_with_caller_precedence(t, b, expr->operand, c_precedence(expr->kind));
        break;

    case UNARY_NEG:
        EMIT("-");
        transpile_expression_with_caller_precedence(t, b, expr->operand, c_precedence(expr->kind));
        break;

    case UNARY_NOT:
        EMIT("!");
        transpile_expression_with_caller_precedence(t, b, expr->operand, c_precedence(expr->kind));
        break;

    case UNARY_INCREMENT:
        transpile_expression_with_caller_precedence(t, b, expr->operand, c_precedence(expr->kind));
        EMIT("++");
        break;

    case UNARY_DECREMENT:
        transpile_expression_with_caller_precedence(t, b, expr->operand, c_precedence(expr->kind));
        EMIT("--");
        break;

#define CASE_BINARY(expr_kind, symbol)                                                          \
    case expr_kind:                                                                             \
        transpile_expression_with_caller_precedence(t, b, expr->lhs, c_precedence(expr->kind)); \
        EMIT(" " symbol " ");                                                                   \
        transpile_expression_with_caller_precedence(t, b, expr->rhs, c_precedence(expr->kind)); \
                                                                                                \
        break;

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

    if (c_precedence(expr->kind) < caller_precedence)
        EMIT(")");
}

void transpile_expression(Transpiler *t, Book *b, Expression *expr)
{
    transpile_expression_with_caller_precedence(t, b, expr, (int)PRECEDENCE_NONE);
}

void transpile_function_signature(Transpiler *t, Book *b, Function *funct)
{
    transpile_type(t, b, funct->return_type);
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

        transpile_type(t, b, parameter->type);
        EMIT(" ");
        EMIT(GET_C_IDENTITY(parameter));
        i++;
    }

    EMIT(")");
}

// TRANSPILE PROGRAM //

void transpile_declarations_in_block(Transpiler *t, Block *block)
{
    Statement *stmt;
    StatementIterator it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        switch (stmt->kind)
        {

        case CODE_BLOCK:
            transpile_declarations_in_block(t, stmt->block);
            break;

        case FUNCTION_DECLARATION:
        {
            Function *funct = stmt->function;

            if (funct != t->apm->main)
            {
                // Forward declaration
                transpile_function_signature(t, &t->function_declarations, funct);
                emit_line(&t->function_declarations, ";");

                // Function definition
                transpile_function_signature(t, &t->body, funct);
                transpile_code_block(t, &t->body, funct->body);
                emit_line(&t->body, "");
            }

            // Transpile nested declarations
            transpile_declarations_in_block(t, funct->body);
        }
        break;

        case ENUM_TYPE_DECLARATION:
        {
            Book *b = &t->enum_definitions;

            EnumType *enum_type = stmt->enum_type;
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
        break;

        case STRUCT_TYPE_DECLARATION:
        {
            // TODO: Create struct forward declarations

            Book *b = &t->struct_definitions;

            StructType *struct_type = stmt->struct_type;

            EMIT_LINE("typedef struct");
            EMIT_OPEN_BRACE();

            Property *property;
            PropertyIterator it = property_iterator(struct_type->properties);
            size_t i = 0;
            while (property = next_property_iterator(&it))
            {
                transpile_type(t, b, property->type);
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

            // Transpile nested declarations
            transpile_declarations_in_block(t, struct_type->body);
        }
        break;

        case VARIABLE_DECLARATION:
        {
            // NOTE: Right now, I _think_ the only declaration block that exists is the global program scope.
            //       The global variables with an order greater than 1 currently have their initial values set
            //       in the main function.

            // FIXME: Check if the above is still true and/or if there are any issues with this.

            if (!block->declaration_block)
                break;

            Book *b = &t->body;
            Variable *var = stmt->variable;

            transpile_type(t, b, var->type);
            EMIT(" ");
            EMIT(GET_C_IDENTITY(var));

            if (stmt->initial_value && var->order < 2)
            {
                EMIT(" = ");
                transpile_expression(t, b, stmt->initial_value);
            }
            else if (!stmt->initial_value)
            {
                EMIT(" = ");
                transpile_default_value(t, b, var->type);
            }

            EMIT_LINE(";");
        }
        break;

        case IF_SEGMENT:
        case ELSE_IF_SEGMENT:
        case ELSE_SEGMENT:
            transpile_declarations_in_block(t, stmt->body);
            break;

        case BREAK_LOOP:
        case FOR_LOOP:
        case WHILE_LOOP:
            transpile_declarations_in_block(t, stmt->body);
            break;

        case BREAK_STATEMENT:
            break;

        case ASSIGNMENT_STATEMENT:
            break;

        case OUTPUT_STATEMENT:
        case EXPRESSION_STMT:
        case RETURN_STATEMENT:
            break;

        default:
            fatal_error("Could not handle %s statement while transpiling declarations in block.", statement_kind_string(stmt->kind));
            break;
        }
    }
}

void transpile_main_function(Transpiler *t)
{
    Book *b = &t->body;

    EMIT_LINE("int main(int argc, char *argv[])");
    EMIT_OPEN_BRACE();

    // Initialise global variables
    Statement *declaration;
    StatementIterator it;

    size_t order = 2;
    bool is_last_order = false;
    while (!is_last_order)
    {
        is_last_order = true;
        it = statement_iterator(t->apm->program_block->statements);
        while (declaration = next_statement_iterator(&it))
        {
            if (declaration->kind == VARIABLE_DECLARATION && declaration->variable->order == order)
            {
                assert(declaration->initial_value);
                is_last_order = false;

                EMIT(GET_C_IDENTITY(declaration->variable));
                EMIT(" = ");
                transpile_expression(t, b, declaration->initial_value);
                EMIT_LINE(";");
            }
        }
        order++;
    }

    // User main function
    transpile_code_block(t, b, t->apm->main->body);

    EMIT_CLOSE_BRACE();
}

void transpile_program(Transpiler *t)
{
    transpile_declarations_in_block(t, t->apm->program_block);
    transpile_main_function(t);
}

void transpile(Compiler *compiler, Program *apm)
{
    Transpiler transpiler;
    transpiler.apm = apm;
    transpiler.source_text = compiler->source_text;

    init_book(&transpiler.enum_definitions);
    init_book(&transpiler.struct_declarations);
    init_book(&transpiler.struct_definitions);
    init_book(&transpiler.function_declarations);
    init_book(&transpiler.body);

    transpile_program(&transpiler);

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
    fputs("\n", output_file);

    write_book_to_file(&transpiler.enum_definitions, output_file);
    write_book_to_file(&transpiler.struct_declarations, output_file);
    write_book_to_file(&transpiler.struct_definitions, output_file);
    write_book_to_file(&transpiler.function_declarations, output_file);
    write_book_to_file(&transpiler.body, output_file);

    fclose(output_file);
}
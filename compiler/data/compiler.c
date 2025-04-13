#include <stdio.h>
#include "compiler.h"

// Compilation errors
DEFINE_ENUM(LIST_COMPILATION_ERRORS, CompilationErrorCode, compilation_error_code)

void raise_compilation_error(Compiler *c, CompilationErrorCode code, substr str)
{
    if (c->error_count == c->error_capacity)
    {
        c->error_capacity = c->error_capacity * 2;
        c->errors = (CompilationError *)realloc(c->errors, sizeof(CompilationError) * c->error_capacity);
    }

    c->errors[c->error_count].code = code;
    c->errors[c->error_count].str.pos = str.pos;
    c->errors[c->error_count].str.len = str.len;
    c->error_count++;
}

void determine_error_positions(Compiler *c)
{
    size_t line = 1;
    size_t column = 1;
    size_t pos = 0;

    // TODO: Sort errors by position, both for usability, and so that
    //       the loop below will work correctly!

    CompilationError *error = c->errors;
    for (size_t i = 0; i < c->error_count; pos++)
    {
        while (pos == error->str.pos)
        {
            error->line = line;
            error->column = column;
            error = c->errors + ++i;
        }

        const char *character = c->source_text + pos;
        if (*character == '\n')
        {
            line++;
            column = 1;
        }
        else if (*character == '\0')
            break;
        else
            column++;
    }
}

void printf_compilation_error(Compiler *c, size_t index)
{
    CompilationError error = c->errors[index];
    printf("\x1b[34m"
           "%s:%d:%d\t%s\n"
           "\x1b[0m",
           c->source_path, error.line, error.column, compilation_error_code_string(error.code));

    if (error.str.len > 0)
    {
        size_t err_start = error.str.pos;
        size_t err_end = error.str.pos + error.str.len - 1;

        size_t line_start = err_start;
        while (line_start > 0 && c->source_text[line_start] != '\n')
            line_start--;
        if (c->source_text[line_start] == '\n')
            line_start++;

        size_t line_end = err_end;
        while (c->source_text[line_end] != '\0' && c->source_text[line_end] != '\n')
            line_end++;
        line_end--;

        printf("%.*s", err_start - line_start, c->source_text + line_start);
        printf("\x1b[31m");
        printf("%.*s", err_end - err_start + 1, c->source_text + err_start);
        printf("\x1b[0m");
        printf("%.*s", line_end - err_end, c->source_text + err_end + 1);
        printf("\n");

        for (size_t i = line_start; i < err_start; i++)
            printf(" ");

        printf("\x1b[31m");
        if (err_start == err_end)
            printf("^");
        else
            for (size_t i = err_start; i <= err_end; i++)
                printf("~");
        printf("\x1b[0m");
        printf("\n\n");
    }
}

// Parser status
DEFINE_ENUM(LIST_PARSE_STATUS, ParseStatus, parse_status)

// Compiler
void init_compiler(Compiler *c)
{
    c->token_capacity = 64;
    c->token_count = 0;
    c->tokens = (Token *)malloc(sizeof(Token) * c->token_capacity);

    c->error_capacity = 8;
    c->error_count = 0;
    c->errors = (CompilationError *)malloc(sizeof(CompilationError) * c->error_capacity);
}

#include "compiler.h"
#include <stdlib.h>

// Compilation errors
DEFINE_ENUM(LIST_COMPILATION_ERRORS, CompilationErrorCode, compilation_error_code)

void raise_compilation_error(Compiler *c, CompilationErrorCode code, size_t pos)
{
    if (c->error_count == c->error_capacity)
    {
        c->error_capacity = c->error_capacity * 2;
        c->errors = (CompilationError *)realloc(c->errors, sizeof(CompilationError) * c->error_capacity);
    }

    c->errors[c->error_count].code = code;
    c->errors[c->error_count].pos = pos;
    c->error_count++;
}

void determine_error_positions(Compiler *c)
{
    size_t line = 1;
    size_t column = 1;
    size_t pos = 0;

    CompilationError *error = c->errors;
    for (size_t i = 0; i < c->error_count; pos++)
    {
        while (pos == error->pos)
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

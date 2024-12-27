#ifndef COMPILER_H
#define COMPILER_H

#include "token.h"
#include <stdlib.h>

typedef struct
{
    // Source and tokenise
    const char *source_path;
    const char *source_text;
    TokenArray token_array;

    // Parse
    size_t next_token;
} Compiler;

#endif
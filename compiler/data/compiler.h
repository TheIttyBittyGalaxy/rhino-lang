#ifndef COMPILER_H
#define COMPILER_H

#include "macro.h"
#include "token.h"
#include <stdlib.h>

// Compilation error code
#define LIST_COMPILATION_ERRORS(MACRO)                              \
    MACRO(INVALID_COMPILATION_ERROR)                                \
                                                                    \
    /* PARSER ERRORS */                                             \
    MACRO(EXPECTED_EXPRESSION)                                      \
    MACRO(EXPECTED_STATEMENT)                                       \
    MACRO(EXPECTED_FUNCTION)                                        \
                                                                    \
    /* NOTE: These should be synced up to LIST_TOKENS in token.h */ \
    MACRO(EXPECTED_INVALID_TOKEN)                                   \
    MACRO(EXPECTED_SEMI_COLON)                                      \
    MACRO(EXPECTED_COLON)                                           \
    MACRO(EXPECTED_PAREN_L)                                         \
    MACRO(EXPECTED_PAREN_R)                                         \
    MACRO(EXPECTED_CURLY_L)                                         \
    MACRO(EXPECTED_CURLY_R)                                         \
    MACRO(EXPECTED_ARROW_L)                                         \
    MACRO(EXPECTED_ARROW_R)                                         \
    MACRO(EXPECTED_ARROW_L_EQUAL)                                   \
    MACRO(EXPECTED_ARROW_R_EQUAL)                                   \
    MACRO(EXPECTED_KEYWORD_TRUE)                                    \
    MACRO(EXPECTED_KEYWORD_FALSE)                                   \
    MACRO(EXPECTED_KEYWORD_ELSE)                                    \
    MACRO(EXPECTED_KEYWORD_FN)                                      \
    MACRO(EXPECTED_KEYWORD_FOR)                                     \
    MACRO(EXPECTED_KEYWORD_IF)                                      \
    MACRO(EXPECTED_KEYWORD_LOOP)                                    \
    MACRO(EXPECTED_KEYWORD_WHILE)                                   \
    MACRO(EXPECTED_IDENTITY)                                        \
    MACRO(EXPECTED_NUMBER)                                          \
    MACRO(EXPECTED_STRING)                                          \
    MACRO(EXPECTED_BROKEN_STRING)                                   \
    MACRO(EXPECTED_END_OF_FILE)                                     \
                                                                    \
    /* ANALYSIS ERRORS */                                           \
    MACRO(NO_MAIN_FUNCTION)                                         \
    MACRO(EXPRESSION_IS_NOT_A_FUNCTION)                             \
    MACRO(FUNCTION_DOES_NOT_EXIST)

DECLARE_ENUM(LIST_COMPILATION_ERRORS, CompilationErrorCode, compilation_error_code)

// Compilation error
typedef struct
{
    CompilationErrorCode code;
    size_t pos;
    size_t line;
    size_t column;
} CompilationError;

// Parser status
#define LIST_PARSE_STATUS(MACRO) \
    MACRO(OKAY)                  \
    MACRO(RECOVERED)             \
    MACRO(PANIC)

DECLARE_ENUM(LIST_PARSE_STATUS, ParseStatus, parse_status)

// Compiler
typedef struct
{
    // Source
    const char *source_path;
    const char *source_text;

    // Tokenize
    // TokenArray
    Token *tokens;
    size_t token_capacity;
    size_t token_count;

    // Parse
    size_t next_token;
    ParseStatus parse_status;

    // Errors
    CompilationError *errors;
    size_t error_count;
    size_t error_capacity;
} Compiler;

void init_compiler(Compiler *c);

void raise_compilation_error(Compiler *c, CompilationErrorCode code, size_t pos);
void determine_error_positions(Compiler *c);

#endif
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "tokenise.h"

// UTILITY FUNCTIONS

bool is_word(const char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

bool is_digit(const char c)
{
    return (c >= '0' && c <= '9');
}

bool is_word_or_digit(const char c)
{
    return is_word(c) || is_digit(c);
}

// TOKENISE //

#define ONE_CHAR(the_char, token_kind) \
    case the_char:                     \
        kind = token_kind;             \
        c++;                           \
        break;

#define ONE_OR_TWO_CHAR(first_char, first_kind, second_char, second_kind) \
    case first_char:                                                      \
        kind = first_kind;                                                \
        c++;                                                              \
        if (*c == second_char)                                            \
        {                                                                 \
            kind = second_kind;                                           \
            c++;                                                          \
        }                                                                 \
        break;

#define IF_KEYWORD_ELSE(the_word, token_kind)                  \
    if (strncmp(start, the_word, (sizeof(the_word) - 1)) == 0) \
        kind = token_kind;                                     \
    else

void tokenise(Compiler *compiler)
{
    TokenArray *token_array = &compiler->token_array;
    token_array->capacity = 64;
    token_array->count = 0;
    token_array->tokens = (Token *)malloc(sizeof(Token) * token_array->capacity);

    const char *c = compiler->source_text;
    while (true)
    {
        const char *const start = c;
        TokenKind kind = INVALID_TOKEN;

        switch (*c)
        {
            ONE_CHAR(';', SEMI_COLON)
            ONE_CHAR(':', COLON)
            ONE_CHAR('(', PAREN_L)
            ONE_CHAR(')', PAREN_R)
            ONE_CHAR('{', CURLY_L)
            ONE_CHAR('}', CURLY_R)
            ONE_OR_TWO_CHAR('<', ARROW_L, '=', ARROW_L_EQUAL)
            ONE_OR_TWO_CHAR('>', ARROW_R, '=', ARROW_R_EQUAL)

        case '\0':
            kind = END_OF_FILE;
            break;

        case ' ':
        case '\t':
        case '\n':
        case '\r':
            c++;
            continue; // Do not emit a token

        default:

            if (is_digit(*c))
            {
                kind = NUMBER;
                while (is_digit(*c))
                    c++;
            }

            else if (is_word(*c))
            {
                while (is_word_or_digit(*c))
                    c++;

                IF_KEYWORD_ELSE("true", KEYWORD_TRUE)
                IF_KEYWORD_ELSE("false", KEYWORD_FALSE)
                IF_KEYWORD_ELSE("for", KEYWORD_FOR)
                IF_KEYWORD_ELSE("if", KEYWORD_IF)
                IF_KEYWORD_ELSE("loop", KEYWORD_LOOP)
                IF_KEYWORD_ELSE("while", KEYWORD_WHILE)
                kind = IDENTITY; // This is under the "else" of the above macro
            }

            else if (*c == '"')
            {
                kind = STRING;
                while (true)
                {
                    c++;
                    if (*c == '"')
                    {
                        c++;
                        break;
                    }

                    if (*c == '\0' || *c == '\n' || *c == '\r')
                    {
                        kind = BROKEN_STRING;
                        break;
                    }
                }
            }

            else
            {
                c++; // Proceed with invalid token
            }
        }

        if (token_array->count == token_array->capacity)
        {
            size_t new_capacity = token_array->capacity * 2;
            token_array->tokens = (Token *)realloc(token_array->tokens, sizeof(Token) * new_capacity);
            token_array->capacity = new_capacity;
        }

        token_array->tokens[token_array->count].kind = kind;
        token_array->tokens[token_array->count].str.pos = start - compiler->source_text;
        token_array->tokens[token_array->count].str.len = c - start;
        token_array->count++;

        if (kind == END_OF_FILE)
            break;
    }
}

#ifndef TOKEN_H
#define TOKEN_H

#include "macro.h"
#include "substr.h"
#include <stdlib.h>

#define LIST_TOKENS(MACRO) \
    MACRO(INVALID_TOKEN)   \
                           \
    MACRO(SEMI_COLON)      \
    MACRO(COLON)           \
                           \
    MACRO(PAREN_L)         \
    MACRO(PAREN_R)         \
                           \
    MACRO(CURLY_L)         \
    MACRO(CURLY_R)         \
                           \
    MACRO(ARROW_L)         \
    MACRO(ARROW_R)         \
    MACRO(ARROW_L_EQUAL)   \
    MACRO(ARROW_R_EQUAL)   \
                           \
    MACRO(KEYWORD_TRUE)    \
    MACRO(KEYWORD_FALSE)   \
                           \
    MACRO(KEYWORD_FOR)     \
    MACRO(KEYWORD_IF)      \
    MACRO(KEYWORD_LOOP)    \
    MACRO(KEYWORD_WHILE)   \
                           \
    MACRO(IDENTITY)        \
    MACRO(NUMBER)          \
    MACRO(STRING)          \
    MACRO(BROKEN_STRING)   \
                           \
    MACRO(END_OF_FILE)

// Token
DECLARE_ENUM(LIST_TOKENS, TokenKind, token_kind)

typedef struct
{
    TokenKind kind;
    substr str;
} Token;

// TokenArray
typedef struct
{
    Token *tokens;
    size_t capacity;
    size_t count;
} TokenArray;

#endif
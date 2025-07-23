#ifndef TOKEN_H
#define TOKEN_H

#include "macro.h"
#include "substr.h"
#include <stdlib.h>

// Token kind
// NOTE: Update compiler.h whenever these are changed.
#define LIST_TOKENS(MACRO) \
    MACRO(INVALID_TOKEN)   \
                           \
    MACRO(SEMI_COLON)      \
    MACRO(COLON)           \
    MACRO(COMMA)           \
    MACRO(DOT)             \
    MACRO(TWO_DOT)         \
                           \
    MACRO(PAREN_L)         \
    MACRO(PAREN_R)         \
    MACRO(CURLY_L)         \
    MACRO(CURLY_R)         \
                           \
    MACRO(EQUAL)           \
    MACRO(PLUS)            \
    MACRO(MINUS)           \
    MACRO(STAR)            \
    MACRO(SLASH)           \
    MACRO(ARROW_L)         \
    MACRO(ARROW_R)         \
    MACRO(EXCLAIM)         \
    MACRO(PERCENTAGE)      \
                           \
    MACRO(TWO_EQUAL)       \
    MACRO(ARROW_L_EQUAL)   \
    MACRO(ARROW_R_EQUAL)   \
    MACRO(EXCLAIM_EQUAL)   \
                           \
    MACRO(KEYWORD_AND)     \
    MACRO(KEYWORD_DEF)     \
    MACRO(KEYWORD_ELSE)    \
    MACRO(KEYWORD_ENUM)    \
    MACRO(KEYWORD_FALSE)   \
    MACRO(KEYWORD_FN)      \
    MACRO(KEYWORD_FOR)     \
    MACRO(KEYWORD_IF)      \
    MACRO(KEYWORD_IN)      \
    MACRO(KEYWORD_LOOP)    \
    MACRO(KEYWORD_OR)      \
    MACRO(KEYWORD_TRUE)    \
    MACRO(KEYWORD_WHILE)   \
                           \
    MACRO(IDENTITY)        \
    MACRO(INTEGER)         \
    MACRO(RATIONAL)        \
    MACRO(STRING)          \
    MACRO(BROKEN_STRING)   \
                           \
    MACRO(END_OF_FILE)

DECLARE_ENUM(LIST_TOKENS, TokenKind, token_kind)

// Token
typedef struct
{
    TokenKind kind;
    substr str;
} Token;

#endif
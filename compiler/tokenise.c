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
        character++;                   \
        break;

#define ONE_OR_TWO_CHAR(first_char, first_kind, second_char, second_kind) \
    case first_char:                                                      \
        kind = first_kind;                                                \
        character++;                                                      \
        if (*character == second_char)                                    \
        {                                                                 \
            kind = second_kind;                                           \
            character++;                                                  \
        }                                                                 \
        break;

#define IF_KEYWORD_ELSE(the_word, token_kind) \
    if (strlen(the_word) == len &&            \
        strncmp(start, the_word, len) == 0)   \
        kind = token_kind;                    \
    else

void tokenise(Compiler *const c)
{
    c->token_count = 0;

    const char *character = c->source_text;
    while (true)
    {
        const char *const start = character;
        TokenKind kind = INVALID_TOKEN;

        // Skip single line comments
        if (*character == '/' && *(character + 1) == '/')
        {
            while (*character != '\n' && *character != '\0')
                character++;
            continue;
        }

        // Parse character
        switch (*character)
        {
            ONE_CHAR(';', SEMI_COLON)
            ONE_CHAR(':', COLON)
            ONE_CHAR(',', COMMA)
            ONE_CHAR('(', PAREN_L)
            ONE_CHAR(')', PAREN_R)
            ONE_CHAR('{', CURLY_L)
            ONE_CHAR('}', CURLY_R)
            ONE_CHAR('+', PLUS)
            ONE_CHAR('-', MINUS)
            ONE_CHAR('*', STAR)
            ONE_CHAR('/', SLASH)
            ONE_CHAR('%', PERCENTAGE)
            ONE_OR_TWO_CHAR('=', EQUAL, '=', TWO_EQUAL)
            ONE_OR_TWO_CHAR('!', EXCLAIM, '=', EXCLAIM_EQUAL)
            ONE_OR_TWO_CHAR('<', ARROW_L, '=', ARROW_L_EQUAL)
            ONE_OR_TWO_CHAR('>', ARROW_R, '=', ARROW_R_EQUAL)
            ONE_OR_TWO_CHAR('.', DOT, '.', TWO_DOT)

        case '\0':
            kind = END_OF_FILE;
            break;

        case ' ':
        case '\t':
        case '\n':
        case '\r':
            character++;
            continue; // Do not emit a token

        default:

            if (is_digit(*character))
            {
                kind = NUMBER;
                while (is_digit(*character))
                    character++;
            }

            else if (is_word(*character))
            {
                while (is_word_or_digit(*character))
                    character++;

                size_t len = character - start;
                IF_KEYWORD_ELSE("and", KEYWORD_AND)
                IF_KEYWORD_ELSE("def", KEYWORD_DEF)
                IF_KEYWORD_ELSE("else", KEYWORD_ELSE)
                IF_KEYWORD_ELSE("enum", KEYWORD_ENUM)
                IF_KEYWORD_ELSE("false", KEYWORD_FALSE)
                IF_KEYWORD_ELSE("fn", KEYWORD_FN)
                IF_KEYWORD_ELSE("for", KEYWORD_FOR)
                IF_KEYWORD_ELSE("if", KEYWORD_IF)
                IF_KEYWORD_ELSE("in", KEYWORD_IN)
                IF_KEYWORD_ELSE("loop", KEYWORD_LOOP)
                IF_KEYWORD_ELSE("or", KEYWORD_OR)
                IF_KEYWORD_ELSE("true", KEYWORD_TRUE)
                IF_KEYWORD_ELSE("while", KEYWORD_WHILE)
                kind = IDENTITY; // This is under the "else" of the above macro
            }

            else if (*character == '"')
            {
                kind = STRING;
                while (true)
                {
                    character++;
                    if (*character == '"')
                    {
                        character++;
                        break;
                    }

                    if (*character == '\0' || *character == '\n' || *character == '\r')
                    {
                        kind = BROKEN_STRING;
                        break;
                    }
                }
            }

            else
            {
                character++; // Proceed with invalid token
            }
        }

        if (c->token_count == c->token_capacity)
        {
            c->token_capacity = c->token_capacity * 2;
            c->tokens = (Token *)realloc(c->tokens, sizeof(Token) * c->token_capacity);
        }

        c->tokens[c->token_count].kind = kind;
        c->tokens[c->token_count].str.pos = start - c->source_text;
        c->tokens[c->token_count].str.len = character - start;
        c->token_count++;

        if (kind == END_OF_FILE)
            break;
    }
}

#include <stdio.h>
#include <stdlib.h>

#include "token.h"

const char *token_kind_string(TokenKind value)
{
    LIST_TOKENS(RETURN_STRING_IF_MATCH)
    return "INVALID_TOKEN_KIND";
}
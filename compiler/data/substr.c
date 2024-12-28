#include "substr.h"
#include <stdio.h>
#include <string.h>

void printf_substr(const char *str, substr sub)
{
    printf("%.*s", sub.len, str + sub.pos);
}

bool substr_match(const char *str, substr a, substr b)
{
    if (a.len != b.len)
        return false;

    if (strncmp(str + a.pos, str + b.pos, a.len) == 0)
        return true;

    return false;
}

bool substr_is(const char *str, substr a, const char *b)
{
    if (a.len != strlen(b))
        return false;

    if (strncmp(str + a.pos, b, a.len) == 0)
        return true;

    return false;
}

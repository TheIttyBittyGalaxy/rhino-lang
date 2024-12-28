#include "substr.h"
#include <stdio.h>
#include <stdbool.h>
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
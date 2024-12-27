#include "substr.h"
#include <stdio.h>

void printf_substr(const char *str, substr sub)
{
    printf("%.*s", sub.len, str + sub.pos);
}

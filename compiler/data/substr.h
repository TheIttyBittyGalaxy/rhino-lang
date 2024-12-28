#ifndef SUBSTR_H
#define SUBSTR_H

#include <stdlib.h>

typedef struct
{
    size_t pos;
    size_t len;
} substr;

void printf_substr(const char *str, substr sub);
bool substr_match(const char *str, substr a, substr b);

#endif
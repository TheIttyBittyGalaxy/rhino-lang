#ifndef SUBSTR_H
#define SUBSTR_H

#include "libs.h"

typedef struct
{
    size_t pos;
    size_t len;
} substr;

void printf_substr(const char *str, substr sub);
bool substr_match(const char *str, substr a, substr b);
bool substr_is(const char *str, substr a, const char *b);

#endif
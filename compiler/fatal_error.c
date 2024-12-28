#include "fatal_error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fatal_error(const char *message, ...)
{
    fprintf(stderr, "A fatal error has occurred. This is an issue with the Rhino compiler, and not with your program.\n");

    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(EXIT_FAILURE);
}
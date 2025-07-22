#include "fatal_error.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

extern bool flag_test_mode;

void fatal_error(const char *message, ...)
{
    if (flag_test_mode)
    {
        printf("FATAL ERROR\n");

        va_list args;
        va_start(args, message);
        vprintf(message, args);
        va_end(args);
        printf("\n");

        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "A fatal error has occurred. This is an issue with the Rhino compiler, and not with your program.\n");

    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(EXIT_FAILURE);
}
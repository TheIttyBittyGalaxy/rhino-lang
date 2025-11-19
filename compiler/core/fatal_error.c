#include "fatal_error.h"

extern bool flag_test_mode;

void fatal_error_at_line(const char *message, const char *file, int line, ...)
{
    if (flag_test_mode)
    {
        printf("FATAL ERROR\n");

        va_list args;
        va_start(args, line);
        vprintf(message, args);
        va_end(args);
        printf("\n");

        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "A fatal error has occurred. This is an issue with the Rhino compiler, and not with your program.\n");
    fprintf(stderr, "%s:%d\n", file, line);

    va_list args;
    va_start(args, line);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(EXIT_FAILURE);
}

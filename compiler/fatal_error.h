#ifndef FATAL_ERROR_H
#define FATAL_ERROR_H

void fatal_error_at_line(const char *message, const char *file, int line, ...);

#define fatal_error(message, ...) fatal_error_at_line(message, __FILE__, __LINE__, ##__VA_ARGS__)

#endif
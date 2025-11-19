#ifndef RUN_ON_STRING_H
#define RUN_ON_STRING_H

#include "libs.h"
#include "substr.h"

typedef struct
{
    char *str;
    size_t size;
    size_t len;
} RunOnString;

void init_run_on_string(RunOnString *run_on, size_t size);

void append_run_on_string_with_length(RunOnString *run_on, const char *str, size_t str_len);
void append_run_on_string_with_terminator(RunOnString *run_on, const char *str);
void append_run_on_string_substr(RunOnString *run_on, const char *source_text, substr sub);

#endif
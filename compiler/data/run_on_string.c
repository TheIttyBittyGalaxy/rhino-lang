#include "run_on_string.h"

void init_run_on_string(RunOnString *run_on, size_t size)
{
    assert(size > 0);

    run_on->str = (char *)malloc(sizeof(char) * size);
    run_on->str[0] = '\0';

    run_on->size = size;
    run_on->len = 0;
}

void append_run_on_string_with_length(RunOnString *run_on, const char *str, size_t str_len)
{
    if (run_on->len + str_len + 1 > run_on->size)
    {
        while (run_on->len + str_len + 1 > run_on->size)
            run_on->size *= 2;

        run_on->str = (char *)realloc(run_on->str, run_on->size);
    }

    memcpy(run_on->str + run_on->len, str, str_len);
    run_on->len += str_len;
    run_on->str[run_on->len] = '\0';
}

void append_run_on_string_with_terminator(RunOnString *run_on, const char *str)
{
    append_run_on_string_with_length(run_on, str, strlen(str));
}

void append_run_on_string_substr(RunOnString *run_on, const char *source_text, substr sub)
{
    append_run_on_string_with_length(run_on, source_text + sub.pos, sub.len);
}

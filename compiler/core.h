#ifndef CORE_H
#define CORE_H

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// UNREACHABLE //

#ifdef __GNUC__
#define unreachable __builtin_unreachable()
#else
#define unreachable
#endif

// UTILITY MACROS //

#define WITH_COMMA(token) token,

#define RETURN_STRING_IF_MATCH(token) \
    if (value == token)               \
        return #token;

// ENUM MACROS //

#define DECLARE_ENUM(LIST_VALUES, camel_case_name, snake_case_name) \
    typedef enum                                                    \
    {                                                               \
        LIST_VALUES(WITH_COMMA)                                     \
    } camel_case_name;                                              \
                                                                    \
    const char *snake_case_name##_string(camel_case_name value);

#define DEFINE_ENUM(LIST_VALUES, camel_case_name, snake_case_name) \
    const char *snake_case_name##_string(camel_case_name value)    \
    {                                                              \
        LIST_VALUES(RETURN_STRING_IF_MATCH)                        \
        return "INVALID[" #snake_case_name "]";                    \
    }

#endif
#ifndef MACRO_H
#define MACRO_H

#define WITH_COMMA(token) token,

#define DECLARE_ENUM(LIST_VALUES, camel_case_name, snake_case_name) \
    typedef enum                                                    \
    {                                                               \
        LIST_VALUES(WITH_COMMA)                                     \
    } camel_case_name;                                              \
                                                                    \
    const char *snake_case_name##_string(camel_case_name value);

#define RETURN_STRING_IF_MATCH(token) \
    if (value == token)               \
        return #token;

#define DEFINE_ENUM(LIST_VALUES, camel_case_name, snake_case_name) \
    const char *snake_case_name##_string(camel_case_name value)    \
    {                                                              \
        LIST_VALUES(RETURN_STRING_IF_MATCH)                        \
        return "INVALID[" #snake_case_name "]";                    \
    }

#endif
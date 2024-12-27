#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

#define DECLARE_LIST_TYPE(camel_case_name, snake_case_name)                           \
    typedef struct                                                                    \
    {                                                                                 \
        camel_case_name *values;                                                      \
        size_t capacity;                                                              \
        size_t count;                                                                 \
    } camel_case_name##List;                                                          \
                                                                                      \
    void init_##snake_case_name##_list(camel_case_name##List *list);                  \
    size_t add_##snake_case_name(camel_case_name##List *list);                        \
    camel_case_name *get_##snake_case_name(camel_case_name##List list, size_t index); \
                                                                                      \
    camel_case_name *get_##snake_case_name##_from_slice(camel_case_name##List list, camel_case_name##Slice slice, size_t index);

#define DEFINE_LIST_TYPE(camel_case_name, snake_case_name)                                                     \
    void init_##snake_case_name##_list(camel_case_name##List *list)                                            \
    {                                                                                                          \
        list->values = (camel_case_name *)malloc(sizeof(camel_case_name));                                     \
        list->capacity = 1;                                                                                    \
        list->count = 0;                                                                                       \
    }                                                                                                          \
                                                                                                               \
    size_t add_##snake_case_name(camel_case_name##List *list)                                                  \
    {                                                                                                          \
        if (list->count == list->capacity)                                                                     \
        {                                                                                                      \
            list->capacity *= 2;                                                                               \
            list->values = (camel_case_name *)realloc(list->values, sizeof(camel_case_name) * list->capacity); \
        }                                                                                                      \
        return list->count++;                                                                                  \
    }                                                                                                          \
                                                                                                               \
    camel_case_name *get_##snake_case_name(camel_case_name##List list, size_t index)                           \
    {                                                                                                          \
        /* FIXME: Check the index is not out of range*/                                                        \
        return list.values + index;                                                                            \
    }

#define DECLARE_SLICE_TYPE(camel_case_name, snake_case_name) \
    typedef struct                                           \
    {                                                        \
        size_t first;                                        \
        size_t count;                                        \
    } camel_case_name##Slice;

#define DEFINE_SLICE_TYPE(camel_case_name, snake_case_name)                                                                     \
    camel_case_name *get_##snake_case_name##_from_slice(camel_case_name##List list, camel_case_name##Slice slice, size_t index) \
    {                                                                                                                           \
        /* FIXME: Check the index is not out of range*/                                                                         \
        return list.values + slice.first + index;                                                                               \
    }

#endif
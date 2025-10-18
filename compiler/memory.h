#ifndef MEMORY_H
#define MEMORY_H

// Code based on: https://upprsk.github.io/blog/custom-c-allocators/
// Accessed: 2025-10-18
// Thanks Lucas!

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define ALIGN_UP(ptr, align) (uint8_t *)((uintptr_t)(ptr) + ((align) - 1) & ~(align - 1));

typedef struct Allocator Allocator;
typedef struct Bucket Bucket;

struct Allocator
{
    Allocator *parent;
    size_t bucket_size;

    Bucket *first;
    Bucket *current;
};

struct Bucket
{
    struct Bucket *next;
    uint8_t *head;
    uint8_t *buffer_end;
    uint8_t buffer[];
};

void init_allocator(Allocator *allocator, Allocator *parent, size_t bucket_size);
void *allocate(Allocator *allocator, size_t size, size_t align);

#define ALLOCATE(T, allocator) (T *)allocate(allocator, sizeof(T), alignof(T))

#define DECLARE_LIST_ALLOCATOR(T, type_name)                                                    \
    typedef struct                                                                              \
    {                                                                                           \
        Allocator allocator;                                                                    \
        size_t count;                                                                           \
    } T##List;                                                                                  \
                                                                                                \
    typedef struct                                                                              \
    {                                                                                           \
        Bucket *bucket;                                                                         \
        size_t index;                                                                           \
        size_t remaining;                                                                       \
    } T##Iterator;                                                                              \
                                                                                                \
    void init_##type_name##_list(T##List *list, Allocator *allocator, size_t items_per_bucket); \
    T *append_##type_name(T##List *list);                                                       \
                                                                                                \
    T##Iterator type_name##_iterator(T##List *list);                                            \
    T *next_##type_name##_iterator(T##Iterator *it);

#define DEFINE_LIST_ALLOCATOR(T, type_name)                                                    \
    void init_##type_name##_list(T##List *list, Allocator *allocator, size_t items_per_bucket) \
    {                                                                                          \
        list->count = 0;                                                                       \
        /* FIXME: This +1 is essentially a hack to account for situations when the alignment   \
                  of T causes it to not start allocations from the start of the buffer*/       \
        size_t bucket_size = (items_per_bucket + 1) * sizeof(T);                               \
        init_allocator(&list->allocator, allocator, bucket_size);                              \
    }                                                                                          \
                                                                                               \
    T *append_##type_name(T##List *list)                                                       \
    {                                                                                          \
        list->count++;                                                                         \
        return (T *)allocate(&list->allocator, sizeof(T), alignof(T));                         \
    }                                                                                          \
                                                                                               \
    T##Iterator type_name##_iterator(T##List *list)                                            \
    {                                                                                          \
        return (T##Iterator){                                                                  \
            .bucket = list->allocator.first,                                                   \
            .index = 0,                                                                        \
            .remaining = list->count,                                                          \
        };                                                                                     \
    }                                                                                          \
                                                                                               \
    T *next_##type_name##_iterator(T##Iterator *it)                                            \
    {                                                                                          \
        if (it->remaining == 0)                                                                \
            return NULL;                                                                       \
        it->remaining--;                                                                       \
                                                                                               \
        Bucket *bucket = it->bucket;                                                           \
        uint8_t *aligned_buffer = ALIGN_UP(bucket->buffer, alignof(T));                        \
        uint8_t *item = aligned_buffer + (sizeof(T) * it->index++);                            \
                                                                                               \
        if (item + sizeof(T) > bucket->buffer_end)                                             \
        {                                                                                      \
            it->bucket = bucket->next;                                                         \
            it->index = 0;                                                                     \
            aligned_buffer = ALIGN_UP(it->bucket->buffer, alignof(T));                         \
            return (T *)aligned_buffer;                                                        \
        }                                                                                      \
                                                                                               \
        return (T *)item;                                                                      \
    }

#endif
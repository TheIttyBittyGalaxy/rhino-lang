#ifndef MEMORY_H
#define MEMORY_H

// Code based on: https://upprsk.github.io/blog/custom-c-allocators/
// Accessed: 2025-10-18
// Thanks Lucas!

#include "libs.h"

#define ALIGN_UP(ptr, align) (uint8_t *)((uintptr_t)(ptr) + ((align) - 1) & ~(align - 1))

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

#define DECLARE_LIST_ALLOCATOR(T, type_name)                                                                  \
    typedef struct                                                                                            \
    {                                                                                                         \
        Allocator allocator;                                                                                  \
        size_t count;                                                                                         \
    } T##ListAllocator;                                                                                       \
                                                                                                              \
    typedef struct                                                                                            \
    {                                                                                                         \
        Bucket *first;                                                                                        \
        size_t index;                                                                                         \
        size_t count;                                                                                         \
    } T##List;                                                                                                \
                                                                                                              \
    typedef struct                                                                                            \
    {                                                                                                         \
        Bucket *bucket;                                                                                       \
        size_t index;                                                                                         \
        size_t remaining;                                                                                     \
    } T##Iterator;                                                                                            \
                                                                                                              \
    void init_##type_name##_list_allocator(T##ListAllocator *list, Allocator *allocator, size_t bucket_size); \
    T *append_##type_name(T##ListAllocator *list_allocator);                                                  \
    T##List get_##type_name##_list(T##ListAllocator list_allocator);                                          \
                                                                                                              \
    T##Iterator type_name##_iterator(T##List list);                                                           \
    T *next_##type_name##_iterator(T##Iterator *it);                                                          \
    T *get_##type_name(T##List list, size_t i);

#define DEFINE_LIST_ALLOCATOR(T, type_name)                                                                  \
    void init_##type_name##_list_allocator(T##ListAllocator *list, Allocator *allocator, size_t bucket_size) \
    {                                                                                                        \
        list->count = 0;                                                                                     \
        init_allocator(&list->allocator, allocator, bucket_size);                                            \
    }                                                                                                        \
                                                                                                             \
    T *append_##type_name(T##ListAllocator *list_allocator)                                                  \
    {                                                                                                        \
        list_allocator->count++;                                                                             \
        return (T *)allocate(&list_allocator->allocator, sizeof(T), alignof(T));                             \
    }                                                                                                        \
                                                                                                             \
    T##List get_##type_name##_list(T##ListAllocator list_allocator)                                          \
    {                                                                                                        \
        return (T##List){                                                                                    \
            .first = list_allocator.allocator.first,                                                         \
            .index = 0,                                                                                      \
            .count = list_allocator.count,                                                                   \
        };                                                                                                   \
    }                                                                                                        \
                                                                                                             \
    T##Iterator type_name##_iterator(T##List list)                                                           \
    {                                                                                                        \
        return (T##Iterator){                                                                                \
            .bucket = list.first,                                                                            \
            .index = list.index,                                                                             \
            .remaining = list.count,                                                                         \
        };                                                                                                   \
    }                                                                                                        \
                                                                                                             \
    T *next_##type_name##_iterator(T##Iterator *it)                                                          \
    {                                                                                                        \
        if (it->remaining == 0)                                                                              \
            return NULL;                                                                                     \
        it->remaining--;                                                                                     \
                                                                                                             \
        Bucket *bucket = it->bucket;                                                                         \
        uint8_t *aligned_buffer = ALIGN_UP(bucket->buffer, alignof(T));                                      \
        uint8_t *item = aligned_buffer + (sizeof(T) * it->index++);                                          \
                                                                                                             \
        if (item + sizeof(T) > bucket->buffer_end)                                                           \
        {                                                                                                    \
            it->bucket = bucket->next;                                                                       \
            it->index = 1;                                                                                   \
            aligned_buffer = ALIGN_UP(it->bucket->buffer, alignof(T));                                       \
            return (T *)aligned_buffer;                                                                      \
        }                                                                                                    \
                                                                                                             \
        return (T *)item;                                                                                    \
    }                                                                                                        \
                                                                                                             \
    T *get_##type_name(T##List list, size_t i)                                                               \
    {                                                                                                        \
        if (i >= list.count)                                                                                 \
            return NULL;                                                                                     \
                                                                                                             \
        T##Iterator it = type_name##_iterator(list);                                                         \
        T *item = next_##type_name##_iterator(&it);                                                          \
        while (i-- > 0)                                                                                      \
            item = next_##type_name##_iterator(&it);                                                         \
                                                                                                             \
        return item;                                                                                         \
    }

#endif
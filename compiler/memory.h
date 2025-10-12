#ifndef MEMORY_H
#define MEMORY_H

void *allocate_bucket();

#define BUCKET_SIZE 1024

#define ITEMS_PER_BUCKET(T) (BUCKET_SIZE - sizeof(size_t)) / sizeof(T)

#define DECLARE_ALLOCATOR(T, snake_name) \
    T *new_##snake_name();

#define DEFINE_ALLOCATOR(T, snake_name)                                                    \
    typedef struct                                                                         \
    {                                                                                      \
        T item[ITEMS_PER_BUCKET(T)];                                                       \
        size_t count;                                                                      \
    } T##Bucket;                                                                           \
                                                                                           \
    T##Bucket *active_##snake_name##_bucket = NULL;                                        \
                                                                                           \
    T *new_##snake_name()                                                                  \
    {                                                                                      \
        if (                                                                               \
            active_##snake_name##_bucket == NULL ||                                        \
            active_##snake_name##_bucket->count == ITEMS_PER_BUCKET(T))                    \
        {                                                                                  \
            active_##snake_name##_bucket = (T##Bucket *)allocate_bucket();                 \
            active_##snake_name##_bucket->count = 0;                                       \
        }                                                                                  \
                                                                                           \
        return &active_##snake_name##_bucket->item[active_##snake_name##_bucket->count++]; \
    }

#endif
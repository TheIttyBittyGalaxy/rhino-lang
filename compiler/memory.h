#ifndef MEMORY_H
#define MEMORY_H

void *allocate_bucket();

#define BUCKET_SIZE 1024

// FIXME: I'm not confident that this formula correctly accounts for alignment?
#define ITEMS_PER_BUCKET(T) (BUCKET_SIZE - sizeof(size_t) - sizeof(void *)) / sizeof(T)

#define DECLARE_ALLOCATOR(T, snake_name)         \
    void init_##snake_name##_allocator();        \
    T *new_##snake_name();                       \
                                                 \
    typedef struct                               \
    {                                            \
        size_t index;                            \
        void *bucket;                            \
    } T##Iterator;                               \
                                                 \
    T##Iterator begin_##snake_name##_iterator(); \
    T *advance_##snake_name##_iterator(T##Iterator *it);

#define DEFINE_ALLOCATOR(T, snake_name)                                                \
    typedef struct T##Bucket T##Bucket;                                                \
    struct T##Bucket                                                                   \
    {                                                                                  \
        T item[ITEMS_PER_BUCKET(T)];                                                   \
        size_t count;                                                                  \
        T##Bucket *next;                                                               \
    };                                                                                 \
                                                                                       \
    T##Bucket *first_##snake_name##_bucket = NULL;                                     \
    T##Bucket *next_##snake_name##_bucket = NULL;                                      \
                                                                                       \
    void init_##snake_name##_allocator()                                               \
    {                                                                                  \
        first_##snake_name##_bucket = (T##Bucket *)allocate_bucket();                  \
        first_##snake_name##_bucket->count = 0;                                        \
        first_##snake_name##_bucket->next = NULL;                                      \
                                                                                       \
        next_##snake_name##_bucket = first_##snake_name##_bucket;                      \
    }                                                                                  \
                                                                                       \
    T *new_##snake_name()                                                              \
    {                                                                                  \
        assert(next_##snake_name##_bucket != NULL);                                    \
                                                                                       \
        if (next_##snake_name##_bucket->count == ITEMS_PER_BUCKET(T))                  \
        {                                                                              \
            T##Bucket *next_bucket = (T##Bucket *)allocate_bucket();                   \
            next_bucket->count = 0;                                                    \
            next_bucket->next = NULL;                                                  \
                                                                                       \
            next_##snake_name##_bucket->next = next_bucket;                            \
            next_##snake_name##_bucket = next_bucket;                                  \
        }                                                                              \
                                                                                       \
        return &next_##snake_name##_bucket->item[next_##snake_name##_bucket->count++]; \
    }                                                                                  \
                                                                                       \
    T##Iterator begin_##snake_name##_iterator()                                        \
    {                                                                                  \
        return {                                                                       \
            .index = 0,                                                                \
            .bucket = (void *)first_##snake_name##_bucket,                             \
        };                                                                             \
    }                                                                                  \
                                                                                       \
    T *advance_##snake_name##_iterator(T##Iterator *it)                                \
    {                                                                                  \
        if (it->bucket == NULL)                                                        \
            return NULL;                                                               \
                                                                                       \
        T##Bucket *bucket = (T##Bucket *)(it->bucket);                                 \
        if (it->index == bucket->count)                                                \
        {                                                                              \
            it->bucket = (void *)bucket->next;                                         \
            it->index = 0;                                                             \
            if (it->bucket == NULL)                                                    \
                return NULL;                                                           \
        }                                                                              \
                                                                                       \
        return &((T##Bucket *)it->bucket)->item[it->index++];                          \
    }

#endif
#include "memory.h"

#define ALIGN_UP(ptr, align) (uint8_t *)((uintptr_t)(ptr) + ((align) - 1) & ~(align - 1))

// ALLOCATORS //

Bucket *next_available_bucket = NULL;

void allocate_buckets()
{
    uint8_t *new_bucket_array = (uint8_t *)malloc(BUCKET_SIZE * BUCKETS_PER_BLOCK);
    assert(new_bucket_array);

    for (size_t i = 0; i < BUCKETS_PER_BLOCK; i++)
    {
        Bucket *bucket = (Bucket *)(new_bucket_array + BUCKET_SIZE * i);

        bucket->tail = (uint8_t *)bucket + BUCKET_SIZE;

        if (i < BUCKETS_PER_BLOCK - 1)
        {
            Bucket *next = (Bucket *)(new_bucket_array + BUCKET_SIZE * (i + 1));
            bucket->next = next;
        }
        else
        {
            bucket->next = NULL;
        }
    }

    next_available_bucket = (Bucket *)new_bucket_array;
}

Bucket *acquire_bucket()
{
    Bucket *bucket;
    if (!next_available_bucket)
        allocate_buckets();

    bucket = next_available_bucket;
    next_available_bucket = bucket->next;

    bucket->head = bucket->data;
    bucket->next = NULL;
    return bucket;
}

void release_bucket(Bucket *bucket)
{
    Bucket *start_of_chain = next_available_bucket;
    next_available_bucket = bucket;

    while (bucket->next)
        bucket = bucket->next;
    bucket->next = start_of_chain;
}

void init_allocator(Allocator *allocator)
{
    allocator->first = NULL;
    allocator->current = NULL;
}

void *allocate_chunk(Allocator *allocator, size_t size, size_t align)
{
    Bucket *bucket = allocator->current;
    if (!bucket)
    {
        bucket = acquire_bucket();
        allocator->first = bucket;
        allocator->current = bucket;
    }

    while (true)
    {
        uint8_t *chunk_start = ALIGN_UP(bucket->head, align);
        uint8_t *chunk_end = chunk_start + size;

        if (chunk_end <= bucket->tail)
        {
            bucket->head = chunk_end;
            return chunk_start;
        }

        Bucket *next = acquire_bucket();
        if (bucket->tail == (uint8_t *)next)
        {
            bucket->tail = next->tail;
        }
        else
        {
            allocator->current = next;
            bucket->next = next;
            bucket = next;
        }
    }
}

// ITERATORS //

Iterator create_iterator(Bucket *bucket)
{
    if (bucket == NULL)
        return (Iterator){
            .bucket = NULL,
            .head = NULL,
        };

    return (Iterator){
        .bucket = bucket,
        .head = bucket->data,
    };
}

void *advance_iterator(Iterator *it, size_t size, size_t align)
{
    if (it->bucket == NULL)
        return NULL;

    while (true)
    {
        uint8_t *chunk = ALIGN_UP(it->head, align);

        if (chunk + size <= it->bucket->head)
        {
            it->head = chunk + size;
            return (void *)chunk;
        }

        it->bucket = it->bucket->next;
        if (it->bucket == NULL)
            return NULL;

        it->head = it->bucket->data;
    }
}

void *get_chunk(Bucket *bucket, size_t i, size_t size, size_t align)
{
    Iterator it = create_iterator(bucket);
    for (size_t j = 0; j < i; j++)
        advance_iterator(&it, size, align);
    return advance_iterator(&it, size, align);
}

size_t count_chunks(Bucket *bucket, size_t size, size_t align)
{
    size_t count = 0;
    Iterator it = create_iterator(bucket);
    while (advance_iterator(&it, size, align))
        count++;
    return count;
}
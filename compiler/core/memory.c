#include "memory.h"

Bucket *allocate_new_bucket(Allocator *allocator)
{
    Bucket *bucket =
        allocator->parent == NULL
            ? (Bucket *)malloc(allocator->bucket_size)
            : (Bucket *)allocate(allocator->parent, allocator->bucket_size, alignof(Bucket));

    assert(bucket != NULL);

    bucket->next = NULL;
    bucket->buffer_end = ((uint8_t *)bucket) + allocator->bucket_size;
    bucket->head = bucket->buffer;

    return bucket;
}

void *allocate_to_bucket(Bucket *bucket, size_t size, size_t align)
{
    uint8_t *aligned_head = ALIGN_UP(bucket->head, align);

    if (aligned_head + size > bucket->buffer_end)
        return NULL;

    bucket->head = aligned_head + size;
    return aligned_head;
}

void *allocate(Allocator *allocator, size_t size, size_t align)
{
    assert(allocator != NULL);
    assert(allocator->current != NULL);

    void *allocation = allocate_to_bucket(allocator->current, size, align);
    if (!allocation)
    {
        Bucket *bucket = allocate_new_bucket(allocator);
        allocator->current->next = bucket;
        allocator->current = bucket;
        allocation = allocate_to_bucket(bucket, size, align);
    }

    assert(allocation != NULL);
    return allocation;
}

void init_allocator(Allocator *allocator, Allocator *parent, size_t bucket_size)
{
    if (parent != NULL)
        assert(bucket_size <= parent->bucket_size - sizeof(Bucket));

    allocator->parent = parent;
    allocator->bucket_size = bucket_size;

    allocator->first = allocate_new_bucket(allocator);
    allocator->current = allocator->first;
}
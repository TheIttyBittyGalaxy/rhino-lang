#ifndef MEMORY_H
#define MEMORY_H

#include "libs.h"

// ALLOCATORS //

#define BUCKETS_PER_BLOCK 64
#define BUCKET_SIZE 128

typedef struct Bucket Bucket;
typedef struct Allocator Allocator;

struct Bucket
{
    Bucket *next;
    uint8_t *head;
    uint8_t *tail;
    uint8_t data[];
};

struct Allocator
{
    Bucket *first;
    Bucket *current;
};

extern Bucket *next_available_bucket;

void init_allocator(Allocator *allocator);

void *allocate_chunk(Allocator *allocator, size_t size, size_t align);
#define allocate(allocator, T) (T *)allocate_chunk(allocator, sizeof(T), alignof(T))

// ITERATORS //

typedef struct Iterator Iterator;

struct Iterator
{
    Bucket *bucket;
    uint8_t *head;
};

Iterator create_iterator(Bucket *bucket);

void *advance_iterator(Iterator *it, size_t size, size_t align);
#define advance_iterator_of(it, T) ((T *)advance_iterator(it, sizeof(T), alignof(T)))

void *get_chunk(Bucket *bucket, size_t i, size_t size, size_t align);
#define get_chunk_of(bucket, i, T) ((T *)get_chunk(bucket, i, sizeof(T), alignof(T)))

size_t count_chunks(Bucket *bucket, size_t size, size_t align);
#define count_chunks_of(bucket, T) count_chunks(bucket, sizeof(T), alignof(T))

#endif
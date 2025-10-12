#include "memory.h"
#include <assert.h>
#include <stdlib.h>

void *allocate_bucket()
{
    void *bucket = malloc(BUCKET_SIZE);
    assert(bucket != NULL);
    return bucket;
}
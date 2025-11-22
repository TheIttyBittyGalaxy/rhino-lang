#include "../core/core.h"
#include "../data/apm.h"
#include <inttypes.h>

typedef enum
{
    MEM_UNKNOWN,
    MEM_UNUSED,
    MEM_WASTED,

    MEM_META,
    MEM_TYPE,
    MEM_FUNCTION,
    MEM_BLOCK,
    MEM_STATEMENT,
    MEM_EXPRESSION,

    MEM_TOTAL_ENUMS
} Memtype;

char memchar[MEM_TOTAL_ENUMS] = {'_', ' ', '*', 'M', 'T', 'F', 'B', 'S', 'E'};

typedef struct
{
    uint64_t start;
    size_t size;
    Memtype memtype;
} Memdata;

struct
{
    Memdata *data;
    size_t count;
    size_t capacity;

    uint64_t first;
    uint64_t last;
} Memmap;

void add_mem_data(void *ptr, size_t size, Memtype memtype)
{
    if (Memmap.count == Memmap.capacity)
    {
        Memmap.capacity *= 2;
        Memmap.data = (Memdata *)realloc(Memmap.data, Memmap.capacity * sizeof(Memdata));
    }

    uint64_t start = (uint64_t)ptr;
    if (start < Memmap.first)
        Memmap.first = start;

    uint64_t end = start + size;
    if (end > Memmap.last)
        Memmap.last = end;

    Memmap.data[Memmap.count++] = (Memdata){
        .start = start,
        .size = size,
        .memtype = memtype,
    };
}

void memmap_apm_bucket(Bucket *bucket)
{
    while (bucket)
    {
        add_mem_data((void *)bucket, sizeof(Bucket), MEM_META);
        add_mem_data((void *)bucket->data, bucket->head - bucket->data, MEM_UNKNOWN);
        add_mem_data((void *)bucket->head, bucket->tail - bucket->head, MEM_WASTED);
        bucket = bucket->next;
    }
}

void memmap_expression(Expression *expr);
void memmap_statement(Statement *stmt);
void memmap_block(Block *block);
void memmap_function(Function *funct);

void memmap_expression(Expression *expr)
{
    add_mem_data((void *)expr, sizeof(Expression), MEM_EXPRESSION);

    switch (expr->kind)
    {
    case IDENTITY_LITERAL:
    case NONE_LITERAL:
    case BOOLEAN_LITERAL:
    case INTEGER_LITERAL:
    case FLOAT_LITERAL:
    case STRING_LITERAL:
    case ENUM_VALUE_LITERAL:
        break;

    case VARIABLE_REFERENCE:
    case FUNCTION_REFERENCE:
    case PARAMETER_REFERENCE:
    case TYPE_REFERENCE:
        break;

    case FUNCTION_CALL:
        // TODO: memmap args
        break;

    case INDEX_BY_FIELD:
        memmap_expression(expr->subject);
        break;

    case RANGE_LITERAL:
        memmap_expression(expr->first);
        memmap_expression(expr->last);
        break;

    case NONEABLE_EXPRESSION:

    case UNARY_POS:
    case UNARY_NEG:
    case UNARY_NOT:
    case UNARY_INCREMENT:
    case UNARY_DECREMENT:
        memmap_expression(expr->subject);
        break;

    case BINARY_MULTIPLY:
    case BINARY_DIVIDE:
    case BINARY_REMAINDER:
    case BINARY_ADD:
    case BINARY_SUBTRACT:
    case BINARY_LESS_THAN:
    case BINARY_GREATER_THAN:
    case BINARY_LESS_THAN_EQUAL:
    case BINARY_GREATER_THAN_EQUAL:
    case BINARY_EQUAL:
    case BINARY_NOT_EQUAL:
    case BINARY_LOGICAL_AND:
    case BINARY_LOGICAL_OR:
        memmap_expression(expr->lhs);
        memmap_expression(expr->rhs);
        break;

    case TYPE_CAST:
        memmap_expression(expr->cast_expr);
        break;
    }
}

void memmap_statement(Statement *stmt)
{
    add_mem_data((void *)stmt, sizeof(Statement), MEM_STATEMENT);

    switch (stmt->kind)
    {
    case FUNCTION_DECLARATION:
        memmap_function(stmt->function);
        break;

    case ENUM_TYPE_DECLARATION:
        add_mem_data((void *)stmt->enum_type, sizeof(EnumType), MEM_TYPE);
        break;

    case STRUCT_TYPE_DECLARATION:
        add_mem_data((void *)stmt->struct_type, sizeof(StructType), MEM_TYPE);
        break;

    case VARIABLE_DECLARATION:
        if (stmt->initial_value)
            memmap_expression(stmt->initial_value);
        break;

    case CODE_BLOCK:
        memmap_block(stmt->block);
        break;

    case IF_SEGMENT:
    case ELSE_IF_SEGMENT:
        memmap_expression(stmt->condition);
    case ELSE_SEGMENT:
        memmap_block(stmt->body);
        break;

    case WHILE_LOOP:
        memmap_expression(stmt->condition);
    case BREAK_LOOP:
    case FOR_LOOP:
        memmap_block(stmt->body);
        break;

    case BREAK_STATEMENT:
        break;

    case ASSIGNMENT_STATEMENT:
        memmap_expression(stmt->assignment_lhs);
        memmap_expression(stmt->assignment_rhs);
        break;

    case OUTPUT_STATEMENT:
    case EXPRESSION_STMT:
    case RETURN_STATEMENT:
        if (stmt->expression)
            memmap_expression(stmt->expression);
        break;
    }
}

void memmap_block(Block *block)
{
    memmap_apm_bucket(block->statements.bucket);

    add_mem_data((void *)block, sizeof(Block), MEM_BLOCK);

    Statement *stmt;
    Iterator it = create_iterator(&block->statements);
    while (stmt = advance_iterator_of(&it, Statement))
        memmap_statement(stmt);
}

void memmap_function(Function *funct)
{
    add_mem_data((void *)funct, sizeof(Function), MEM_FUNCTION);

    memmap_block(funct->body);
}

void memmap(Program *apm, Allocator apm_allocator)
{
    Memmap.count = 0;
    Memmap.capacity = 128;
    Memmap.data = (Memdata *)malloc(Memmap.capacity * sizeof(Memdata));

    Memmap.first = 0xFFFFFFFFFFFFFFFF;
    Memmap.last = 0;

    // Empty buckets
    Bucket *bucket = next_available_bucket;
    while (bucket)
    {
        add_mem_data((void *)bucket, sizeof(Bucket), MEM_META);
        add_mem_data((void *)bucket->data, bucket->tail - bucket->data, MEM_UNUSED);
        bucket = bucket->next;
    }

    // Walk apm
    memmap_apm_bucket(apm_allocator.first);
    memmap_block(apm->program_block);

    // Align data to 0
    for (size_t i = 0; i < Memmap.count; i++)
        Memmap.data[i]
            .start = Memmap.data[i].start - Memmap.first;

    // Debug print
    // for (size_t i = 0; i < Memmap.count; i++)
    //     printf("%llu\t%d\t%d\n", Memmap.data[i].start, Memmap.data[i].size, Memmap.data[i].memtype);

    // Create buffer
    size_t N = Memmap.last - Memmap.first;
    size_t T = N + N / BUCKET_SIZE - 1;
    char *buffer = (char *)malloc(T);

    for (size_t i = 0; i < T; i++)
        buffer[i] = T % BUCKET_SIZE == 1023 ? ' ' : '\n';

#define POS(x) ((x) + (x) / BUCKET_SIZE)

    for (size_t i = 0; i < Memmap.count; i++)
    {
        Memdata data = Memmap.data[i];
        uint64_t b1 = data.start;
        uint64_t b2 = b1 + data.size;
        char c = memchar[data.memtype];

        for (uint64_t j = b1; j < b2; j++)
            buffer[POS(j)] = c;
        if (data.memtype >= MEM_META && data.size > 2)
        {
            buffer[POS(b1)] = '[';
            buffer[POS(b2 - 1)] = ']';
        }
    }

    FILE *f = fopen("memmap.txt", "w");
    fprintf(f, "%.*s", T, buffer);
    fclose(f);

#undef POS
}

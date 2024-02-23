#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#define ALLOCATOR_BUCKET_SIZE 10
#include "util.h"



usize mem_align(usize);

struct Arena_Bucket {
    isize point = 0;
    void *data = nullptr;
    isize available(isize);
    void *request(isize, isize);
};

struct Arena_Allocator : Allocator {
    Array<Arena_Bucket> data;
    Allocator *allocator; // the underlying allocator (malloc)
    isize point = 0;
    isize bucket_size;
    void *_alloc(isize) override;
    void *_realloc(void *, isize) override;
    void _free(void *) override;
    void clear_arena();
    void destroy();
    usize ptr_len(void *);
};
struct Default_Allocator : Allocator {
    void *_alloc(isize) override;
    void *_realloc(void *, isize) override;
    void _free(void *) override;
};
Arena_Allocator make_arena_allocator(Allocator *, isize = 10000000, isize = 1);

bool test_allocator();

#endif // ALLOCATOR_H
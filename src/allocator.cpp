#include "allocator.h"
void *Allocator::_alloc(isize) {return nullptr;}
void *Allocator::_realloc(void *, isize) {return nullptr;}
void Allocator::_free(void *) {}

const isize SIZE_OFFSET = sizeof(usize);

// Formula for memory alignment 
// see: https://en.wikipedia.org/wiki/Data_structure_alignment
usize mem_align(usize data) {
    return (data + (SIZE_OFFSET-1)) & -SIZE_OFFSET;
}
Arena_Bucket make_arena_bucket(isize count, Allocator *allocator) {
    Arena_Bucket b = {};
    b.data = allocator->_alloc(count);
    return b;
}

isize Arena_Bucket::available(isize bucket_size) {
    return bucket_size - point;
}
void *Arena_Bucket::request(isize count, isize bucket_size) {
    isize total_count = count + SIZE_OFFSET;
    isize c = available(bucket_size);
    assert(point<=bucket_size);
    if(c-total_count>=0) {
        assert(c>=0);
        u8 *p = (u8 *)data;
        usize *d = (usize *)(p+point);
        *d = count;
        auto curr = point + SIZE_OFFSET;
        point += total_count;            
        return p+curr;
    } else {
        return nullptr;
    }
}
Arena_Allocator make_arena_allocator(Allocator *allocator, isize bucket_size, isize bucket_count) {    
    Arena_Allocator a = {};
    a.bucket_size = bucket_size;
    a.data.resize(bucket_count);
    a.allocator = allocator;
    for_range(i, 0, bucket_count) {
        a.data[i] = make_arena_bucket(bucket_size, allocator);
    }
    return a;
}
void *Arena_Allocator::_alloc(isize count) {
    count = mem_align(count);
    void *ptr = nullptr;
    while(point<data.count) {
        ptr = data[point].request(count, bucket_size);
        if(ptr) break;
        point += 1;
    }
    if(ptr) {
        return ptr;
    }
    // make a new bucket
    data.add(make_arena_bucket(bucket_size, allocator));
    ptr = data[data.count-1].request(count, bucket_size);
    assert(ptr);
    // __builtin_assume_aligned ;
    return ptr;
}
void *Arena_Allocator::_realloc(void *ptr, isize count) {
    if(ptr == nullptr) {
        return this->_alloc(count);
    }
    usize *size_ptr = (usize *)((u8 *)ptr - SIZE_OFFSET);        
    isize size = *size_ptr;
    assert(size > 0);
    auto a = this->_alloc(count);        
    memcpy(a, ptr, size);
    return a;
}
void Arena_Allocator::_free(void *) {}
void Arena_Allocator::clear_arena() {
    
    this->point = 0;
    // data.count is always 1 so the whole resetting is O(1)
    for_range(i, 0, data.count) {
        data[i].point = 0;
    }
}
void Arena_Allocator::destroy() {
    for_range(i, 0, data.count) {
        allocator->_free(data[i].data);
    }
    data.destroy();
}
usize Arena_Allocator::ptr_len(void *ptr) {
    return *(usize *)((u8 *)ptr - SIZE_OFFSET);
}

void *Default_Allocator::_alloc(isize count) {
    return ::malloc(count);
}
void *Default_Allocator::_realloc(void *ptr, isize count) {
    return ::realloc(ptr, count);
}
void Default_Allocator::_free(void *ptr) {
    ::free(ptr);
}

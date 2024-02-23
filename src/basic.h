#ifndef BASIC_H
#define BASIC_H
#include <stdlib.h>

#include <cstddef> // ptrdiff_t
#include <stdint.h>


typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;

typedef uint8_t  byte;

typedef size_t    usize; // u64 on x64 machine
typedef ptrdiff_t isize; // i64 on x64 machine


#ifdef _MSC_VER
	#define MSVC
	#define force_inline __forceinline
#elif __GNUC__
	#define GCC
	#define force_inline __attribute__((always_inline))
#else
	#define force_inline inline
#endif

#if !defined GCC && !defined MSVC
	// static_assert(false);
#endif


void _crash(const char *file_name, int line, const char *msg = "");
#define crash(MSG) _crash(__FILE__, __LINE__, MSG)


#define release_assert(COND, ...) if(!(COND)) {_crash(__FILE__, __LINE__, ##__VA_ARGS__); }
// debug macros
// this can be defined through the build file!
#ifdef XDEBUG
#define assert(COND, ...) if(!(COND)) {_crash(__FILE__, __LINE__, ##__VA_ARGS__); }
#define IS_DEBUG true
#else
#define assert(A, ...)   ((void)0)
#define IS_DEBUG false
#endif

#define if_debug if constexpr(IS_DEBUG)


// python-like: for i in range(0, n); type of i is the same as n
#define for_range(i, s, n) for(decltype(n) i = s; i < n; i += 1)


// the Allocator Interface
struct Allocator {
	virtual void *_alloc(isize);
	virtual void *_realloc(void *, isize);
	virtual void  _free(void *);
};
struct Arena_Allocator;

// Current allocator in use
extern Allocator *global_allocator;
// malloc
extern Allocator *global_default_allocator;
// allocator.h
extern Arena_Allocator *global_arena_allocator;

inline void *mem_alloc(isize count, isize size) {
	assert(count >= 0);
	return global_allocator->_alloc(count*size);
}

inline void *mem_realloc(void *ptr, isize count, isize size) {
	assert(count >= 0);
	return global_allocator->_realloc(ptr, count*size);
}

inline void mem_free(void *ptr) {
	global_allocator->_free(ptr);
}

template<typename T>
inline T *mem_alloc(isize count = 1) {
	T *ptr = (T*)mem_alloc(count, sizeof(T));
	return ptr;
}
#include <new>
// Allocates a single object and uses its default constructor
template<typename T>
inline T *mem_new() {
	void *ptr = mem_alloc(1, sizeof(T));
	// placement new
	T *obj = new(ptr) T();
	return obj;
}

#define carray_len(A) sizeof(A)/sizeof(A[0])
#define println_str(A) {auto S = str(A); println(S); S.destroy();}
#endif // BASIC_H
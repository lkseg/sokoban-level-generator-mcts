#ifndef UTIL_H
#define UTIL_H


#include <stdio.h>
#include <string.h>
#include <iostream>

#include "xmath.h"

#include "basic.h"
#include "ctype.h"




struct String;
struct Data_Average;

template<typename T>
struct Array;
template<typename, i32>
struct Static_Array;

void print();
template<typename T, typename ...Args>
void print(const T &t, Args... args) {
	if constexpr(std::is_same<T, bool>::value) {
		if(t) std::cout<<"true";
		else std::cout<<"false";		
	} else {
		std::cout<<t;
	}
	std::cout<<" ";	
	print(args...);
}

#define println(...) print(__VA_ARGS__, "\n")

std::ostream &operator<<(std::ostream &os, String);
std::ostream &operator<<(std::ostream &os, Vector2);
std::ostream &operator<<(std::ostream &os, Vector2i);


template<typename T>
std::ostream &operator<<(std::ostream &os, Array<T> &e) {
	for_range(i, 0, e.count) {
		print(i, ": ", e[i], "\n");
	}
	return os;
}


template<typename T>
void print_2d(Array<T> &e, isize width, isize height) {
	print("\n");
	for_range(y, 0, height) {
		for_range(x, 0, width) {
			print(e[y*width+x], " ");
		}
		print("\n");
	}
}

template<typename T>
struct Slice {
	isize count = 0;
	T *data = nullptr;
	T &operator[](isize index) {
		assert(0<=index && index<count, "bad index");
		return data[index];	
	}
};


/*
	A simple dynamic Array
	Features:
		get/set:
			value = array[index]
			array[index] = value
		adding an element:
			array.add(value)
		find index of element (<0 if it doesn't exist):
			array.find(value)
		removes an element at index and performs a left shift for ]index,count[:
			array.remove(index)
		memory:
			array.resize(new_count)
			array.reserve(new_capacity)
			array.destroy()
			array.shrink() // reduces used memory to count
		slicing:
			array.slice(start, exclusive_end)
*/

template<typename T>
struct Array {
	isize capacity = 0;
	isize count    = 0;
	T *data        = nullptr;

	force_inline T &operator[](isize index) {
		assert(0<=index && index<count, "bad index");
		return data[index];
	}
	void add(const T &val) {
		if(capacity > count) {
			data[count] = val;
			count += 1;
		} else {

			bool ret = reserve((capacity+1)*2);
			if(!ret) return;
			data[count] = val;
			count += 1;
		}
	}
	void add(const T &&val) {
		add(val);
	}	
	isize find(const T &val) {
		for_range(i, 0, count) {
			if(data[i] == val) {
				return i;
			}
		}
		return -1;
	}
	bool reserve(isize a_capacity) {
		if (a_capacity < 0) {
			a_capacity = 0;
		}
		if(capacity >= a_capacity) {
			return true;
		}
		void *ret = mem_realloc(data, a_capacity, sizeof(T));
		if(ret == nullptr) {
			return false;
		}
		data = (T*)ret;
		capacity = a_capacity;
		return true;
	}
	bool resize(isize a_size) {
		if (a_size < 0) {
			count = 0;
			return true;
		}
		if(a_size > capacity) {
			bool ret = reserve(a_size);
			assert(ret, "bad allocation");
			count = a_size;
			return true;
		}
		count = a_size;
		return true;
	}
	// shrinks the array to the current count
	void shrink() {
		if(capacity==count) {
			return;
		}
		T *new_data = (T*) mem_alloc(count, sizeof(T));

		capacity = count;
		memcpy(new_data, data, count*sizeof(T));
		mem_free(data);
		data = new_data;
	}
	void remove(isize index) {
		assert(0<=index && index<count);
		if(count <= 0) return;
		// left shift
		for_range(i, index, count-1) {
			data[i] = data[i+1];
		}
		count -= 1;
	}
	void remove_match(T &val) {
		isize index = find(val);
		assert(index>=0);
		remove(index);
	}
	void destroy() {
		if(data != nullptr) {
			mem_free(data);
		} 
		capacity = 0;
		count = 0;
		data = nullptr;
	}
		// [inclusive, exclusive[
	Slice<T> slice(isize start, isize end) {
		assert(count>=end && start>=0 && end>=start);
		Slice<T> slice;
		slice.data = data + start;
		slice.count = end-start;
		return slice;
	}
	Slice<T> slice(isize start) {
		return slice(start, count);
	}
};		


template<typename T>	
Array<T> make_array(isize count, isize capacity) {
	assert(capacity>=count && count >= 0 && capacity >= 0);
	Array<T> arr;
	// malloc(0) is implementation defined
	if(capacity > 0) {
		arr.data = (T*) mem_alloc(capacity, sizeof(T));
	} else {
		arr.data = nullptr;
	}
	arr.count = count;
	arr.capacity = capacity;
	return arr;
}
template<typename T>
Array<T> make_array(isize count) {
	return make_array<T>(count, count);
}
template<typename T>
T *make_zeroed_array(isize count) {
	auto self = mem_alloc<T>(count);
	memset(self, 0, sizeof(T) * count);
	return self;
}
template<typename T>
void init_data(Array<T> &arr, T value = T{}) {
	for_range(i, 0, arr.count) {
		arr[i] = value;
	}
}
template<typename T>
void destroy_data(Array<Array<T>> &arr) {
	for_range(i, 0, arr.count) {
		arr[i].destroy();
	}
	arr.destroy();
}
template<typename T>
void destroy_inner(Array<T> &arr) {
	for_range(i, 0, arr.count) {
		arr[i].destroy();
	}
}
template<typename T>
inline Array<T> clone_array(Array<T> &a) {
	auto self = make_array<T>(a.count, a.capacity);
	memcpy(self.data, a.data, a.capacity*sizeof(T));
	return self;
}
template<typename T>
inline T *clone_array(T *ptr, isize count) {
	auto self = mem_alloc<T>(count);
	memcpy(self, ptr, count*sizeof(T));
	return self;
}
template<typename T>
void copy_array(Array<T> &dst, Array<T> &src) {
	dst.count = src.count;
	dst.capacity = src.capacity;
	memcpy(dst.data, src.data, src.capacity*sizeof(T));
}


/*
	A dynamic esque array on the stack that (obviously) can't reallocate
*/

template<typename T, i32 capacity>
struct Static_Array {
	T data[capacity];
	i16 count = 0; // no real reason to go beyond i16 since it would destroy the stack

	T &operator[](isize index) {
		assert(0<=index && index<count, "bad index");
		return data[index];		
	}
	void add(const T &val) {
		if(capacity > count) {
			data[count] = val;
			count += 1;
		}
	}
	void add(const T &&val) {
		add(val);
	}	
	void resize(isize a_size) {
		if (a_size < 0) {
			count = 0;
			return;
		}
		if(a_size > capacity) {
			count = capacity;
			return;
		}
		count = a_size;
		return;
	}
};
/*
	Simple immutable string
	data is always a cstring
*/
struct String {
	const char *data = nullptr;
	isize count = 0;

	// implicit constructor; make_string is the explicit version
	String(const char *);
	String();
	char operator[](isize);
	void destroy();
	bool is_empty();
};
bool operator==(const String &, const String &);
String make_string(const char *);
String make_string(Array<char> &);
String clone_string(const char *);
String clone_string(const char *, isize, isize);
String concat(String, String);

isize string_match(String, String, isize start = 0);

struct Byte_String {
	byte *data = nullptr;
	isize count = 0;
	char operator[](isize);
	void destroy();	
};

#include <random>
extern std::mt19937 g_random_engine;
void set_global_random_engine_seed(u64);
f64 randf_range(f64, f64);
i64 randi_range(i64, i64);

template<typename T>
T clamp(T x, T a, T b) {
	return (x < a)? a : (x > b)? b : x;
}


// If windows.h gets ever included
#undef max
#undef min
template<typename T>
T max(T a, T b) {
	return (a > b)? a : b;
}
template<typename T>
T max(T a, T b, T c) {
	return max(max(a, b), c);
}
template<typename T>
T max(T a, T b, T c, T d) {
	return max(max(a, b, c), d);
}
template<typename T>
usize argmax(T a, T b) {
	return (a > b)? 0 : 1;
}
template<typename T>
usize argmax(T a, T b, T c) {
	isize idx = argmax(a,b);
	if(idx == 0 && a > c) {
		return 0;
	}
	if(b > c) {
		return 1;
	}
	return 2;
}
template<typename T>
T min(T a, T b) {
	return (a < b)? a : b;
}
template<typename T>
T min(T a, T b, T c) {
	return min(min(a, b), c);
}
template<typename T>
T min(T a, T b, T c, T d) {
	return min(min(a, b, c), d);
}
// Actual modulo; '%' is division remainder in c++
template<typename T>
T mod(T a, T b) {
	T rem = a % b;
	return (rem>=0)? rem : rem + b;
}
#define swap_values(A, B) auto _D = A; A = B; B = _D
bool approx(f32, f32);
bool approx(f64, f64);

#include "chrono"

typedef std::chrono::time_point<std::chrono::system_clock> Chrono_Clock;
Chrono_Clock get_time();
f64 time_diff(Chrono_Clock start, Chrono_Clock end);

#endif // UTIL_H

#include "util.h"
#include "settings.h"

u64 alloc_count = 0;
u64 free_count = 0;
Allocator *global_allocator = nullptr;
Allocator *global_default_allocator = nullptr;
Arena_Allocator *global_arena_allocator = nullptr;

void _crash(const char *file_name, int line, const char *msg) {
	println("\n-----------CRASH-----------");
	println("error: ", msg);
	println("file:  ", file_name);
	println("line:  ", line);
	println("---------------------------");
 	abort();
}


void print() {
	std::cout << std::flush;
}
std::ostream &operator<<(std::ostream &os, String v) {
	for_range(i, 0, v.count) {
		os << v[i];
	}
	return os;
}
std::ostream &operator<<(std::ostream &os, Vector2 v) {
	os << "Vector2{" << v.x << "," << v.y << "}";
	return os;
}
std::ostream &operator<<(std::ostream &os, Vector2i v) {
	os << "Vector2i{" << v.x << "," << v.y << "}";
	return os;
}


String::String() {}

String::String(const char *s) {
	if(s == nullptr) {
		this->count = 0;
		this->data=nullptr;
		return;
	}
	this->count = strlen(s);
	this->data = s;
}

char String::operator[](isize index) {
	assert(0<=index && index<count);
	return data[index];	
}

String concat(String a, String b) {
	String string;
	string.count = a.count + b.count;
	// +1 null byte
	char *raw = (char*)mem_alloc(string.count+1, sizeof(char));
 	memcpy(raw, a.data, a.count);
	memcpy(raw+a.count, b.data, b.count);
	raw[string.count] = '\0';
	string.data = (const char *)raw;
	return string;
}
bool String::is_empty() {
	return this->count == 0;
}
// User has to make sure the string isn't on the stack
void String::destroy() {
	if(data) {
		mem_free((void*)data);
	}
}
bool operator==(const String &a, const String &b) {

	if(a.count != b.count) {
		return false;
	}

	//           memcmp returns 0 if equal
	return !bool(memcmp(a.data, b.data, a.count));
}
//String make_string(const char *const)
String make_string(const char *s) {
	String string;
	string.count = strlen(s);
	string.data = s;
	return string;
}
String make_string(Array<char> &arr) {
	String string;
	string.count = arr.count;
	string.data = arr.data;
	return string;
} 
String clone_string(const char *s) {
	String string;
	string.count = strlen(s);
	// char is at _least_ 1byte long so no void* 
	char *raw = (char *)mem_alloc(string.count+1, sizeof(char));
	raw[string.count] = '\0';
	// not null terminated
	string.data = (const char *)memcpy(raw, s, string.count);
	
	return string;
}
String clone_string(const char *s, isize from, isize to) {
	String string;
	assert(to >= from);
	string.count = to-from;
	char *raw = (char *)mem_alloc(string.count+1, sizeof(char));
	raw[string.count] = '\0';
	string.data = (const char *)memcpy(raw, s+from, string.count);
	
	return string;
}

isize string_match(String s, String f, isize start) {
	assert(f.count > 0);
	for_range(i, start, s.count) {
		if(s[i] == f[0]) {
			// no more remaining space
			if(s.count<=i+f.count-1) return -1;
			for_range(j, 0, f.count) {
				if(s[i+j] != f[j]) {
					return -1;
				}
			}
			return i;
		}
	}
	return -1;
}

void Byte_String::destroy() {
	mem_free((void *)data);
}

char Byte_String::operator[](isize index) {
	assert(0<=index && index<count);
	return data[index];	
}


bool approx(f32 a, f32 b) {
	return abs(a-b) < 0.001;
}
bool approx(f64 a, f64 b) {
	return abs(a-b) < 0.001;
}

#include <random>
#include "settings.h"

std::mt19937 g_random_engine(DEFAULT_SEED);

void set_global_random_engine_seed(u64 seed) {
	// ~g_random_engine();
	g_random_engine = std::mt19937(seed);
}

// Random number in range [start, end)
f64 randf_range(f64 start, f64 end) {
	// std::random_device{}()
	// static std::mt19937 r_engine(default_seed);
	std::uniform_real_distribution<f64> r_range(start, end);
	return r_range(g_random_engine);
}


#if MT_RANDOM == true
// Random number in range [start, end]
i64 randi_range(i64 start, i64 end) {
	// static std::mt19937 r_engine(default_seed);
	std::uniform_int_distribution<i64> r_range(start, end);
	return r_range(g_random_engine);
}
#else
// Random number in range [0, end]
i64 randi_range(i64 start, i64 end) {
	// +1 for end]
	// 7-0+1 = 8 for range [0, 8]
	return start + i64(rand()) % (end-start+1);
}
#endif

Chrono_Clock get_time() {
	return std::chrono::system_clock::now();
}
f64 time_diff(Chrono_Clock start, Chrono_Clock end) {
	return std::chrono::duration<f64>(end-start).count();
}

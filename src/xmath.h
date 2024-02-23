#ifndef XMATH_H
#define XMATH_H

// Vector2 and Vector3 from raylib
#include "raylib.h"
#include "basic.h"
#include <math.h>

// struct Vector2 {
// 	f32 x;
// 	f32 y;
// };

// struct Vector3 {
// 	f32 x;
// 	f32 y;
// 	f32 z;
// };


struct Vector2i {
	i32 x;
	i32 y;
};

Vector2 operator+(Vector2, Vector2);
Vector2 operator-(Vector2, Vector2);
Vector2 operator*(Vector2, Vector2);

Vector2 operator/(Vector2, Vector2);
Vector2 operator*(Vector2, f32);
Vector2 operator/(Vector2, f32);

bool operator==(Vector2, Vector2);
bool operator!=(Vector2, Vector2);

Vector2 dot(Vector2, Vector2);


// Vector2i operator+(Vector2i, Vector2i);
inline Vector2i operator+(Vector2i a, Vector2i b) {
	return {a.x+b.x, a.y+b.y};
}
// Returns the inverted vector direction
inline Vector2i inv(Vector2i v) {
	return {-v.x, -v.y};
}
inline i32 manhattan_distance(Vector2i a, Vector2i b) {
	return abs(b.x - a.x) + abs(b.y - a.y);
}
Vector2i operator-(Vector2i, Vector2i);
Vector2i operator*(Vector2i, Vector2i);
Vector2i operator*(i32, Vector2i);
Vector2i operator/(Vector2i, Vector2i);
Vector2i operator/(Vector2i, i32);

inline bool operator==(Vector2i a, Vector2i b) {
	return (a.x == b.x) & (a.y == b.y);
}
bool operator!=(Vector2i, Vector2i);

Vector2i dot(Vector2i, Vector2i);

// Conversions
Vector2 vec2(Vector2i);

#include <limits>
constexpr u8  U8_MAX  = std::numeric_limits<u8 >::max();
constexpr u16 U16_MAX = std::numeric_limits<u16>::max();
constexpr u32 U32_MAX = std::numeric_limits<u32>::max();
constexpr u64 U64_MAX = std::numeric_limits<u64>::max();

constexpr i8  I8_MAX  = std::numeric_limits<i8 >::max();
constexpr i16 I16_MAX = std::numeric_limits<i16>::max();
constexpr i32 I32_MAX = std::numeric_limits<i32>::max();
constexpr i64 I64_MAX = std::numeric_limits<i64>::max();

constexpr f32 F32_MAX = std::numeric_limits<f32>::max();
constexpr f64 F64_MAX = std::numeric_limits<f64>::max();

constexpr isize ISIZE_MAX = std::numeric_limits<isize>::max();
constexpr usize USIZE_MAX = std::numeric_limits<usize>::max();

constexpr u8  U8_MIN  = std::numeric_limits<u8 >::min();
constexpr u16 U16_MIN = std::numeric_limits<u16>::min();
constexpr u32 U32_MIN = std::numeric_limits<u32>::min();
constexpr u64 U64_MIN = std::numeric_limits<u64>::min();

constexpr i8  I8_MIN  = std::numeric_limits<i8 >::min();
constexpr i16 I16_MIN = std::numeric_limits<i16>::min();
constexpr i32 I32_MIN = std::numeric_limits<i32>::min();
constexpr i64 I64_MIN = std::numeric_limits<i64>::min();

// !!! lowest !!! instead of min for floating point numbers
constexpr f32 F32_MIN = std::numeric_limits<f32>::lowest();
constexpr f64 F64_MIN = std::numeric_limits<f64>::lowest();

constexpr isize ISIZE_MIN = std::numeric_limits<isize>::min();
constexpr usize USIZE_MIN = std::numeric_limits<usize>::min();


constexpr f32 F32_EPS = std::numeric_limits<f32>::epsilon();
constexpr f64 F64_EPS = std::numeric_limits<f64>::epsilon();

// #define PI 3.14159265359
#endif // XMATH_H

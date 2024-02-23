#include "xmath.h"

Vector2 operator+(Vector2 a, Vector2 b) {
	return {a.x+b.x, a.y+b.y};
}
Vector2 operator-(Vector2 a, Vector2 b) {
	return {a.x-b.x, a.y-b.y};
}
Vector2 operator*(Vector2 a, Vector2 b) {
	return {a.x*b.x, a.y*b.y};
}
Vector2 operator*(Vector2 a, f32 f) {
	return {a.x*f, a.y*f};
}
Vector2 operator/(Vector2 a, f32 f) {
	return {a.x/f, a.y/f};
}
Vector2 operator/(Vector2 a, Vector2 b) {
	return {a.x/b.x, a.y/b.y};
}

bool operator==(Vector2 a, Vector2 b) {
	return (a.x == b.x) & (a.y == b.y);
}
bool operator!=(Vector2 a, Vector2 b) {
	return (a.x != b.x) | (a.y != b.y);
}


Vector2i operator-(Vector2i a, Vector2i b) {
	return {a.x-b.x, a.y-b.y};
}
Vector2i operator*(Vector2i a, Vector2i b) {
	return {a.x*b.x, a.y*b.y};
}
Vector2i operator*(i32 f, Vector2i a) {
	return {a.x*f, a.y*f};
};
Vector2i operator/(Vector2i a, Vector2i b) {
	return {a.x/b.x, a.y/b.y};
}
Vector2i operator/(Vector2i a, i32 f) {
	return {a.x/f, a.y/f};
}


bool operator!=(Vector2i a, Vector2i b) {
	return (a.x != b.x) | (a.y != b.y);
}



Vector2 vec2(Vector2i v) {
	return {f32(v.x), f32(v.y)};
}
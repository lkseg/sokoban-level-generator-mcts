#ifndef SOKOBAN_H
#define SOKOBAN_H
#include "util.h"


// The structure is as follows:
// The lower 4 bits represent the 'bottom'/'ground' layer 
// i.e. Empty, Goal
// The upper 4 bits represent the 'top'/'collision' layer
// i.e. Empty, Pusher, Box, Block
enum struct Pawn: u8 {
	Empty  = 0, // Not a bit!
	Goal   = 1 << 0,
		
	Pusher = 1 << 4,
	Box  = 1 << 5,
	Block  = 1 << 6,
	
	Pusher_On_Goal = Pusher | Goal,
	Box_On_Goal =  Box  | Goal,


	Mover  = Pusher | Box,
	Collision = Pusher | Box | Block,
	Box_Or_Block = Block | Box,
	
};

// A bunch of bit operations

#define top_layer(X) u8(u8(X) & 0b11110000)
#define bot_layer(X) u8(u8(X) & 0b00001111)


#define pawn_has_collision(X)    bool(u8(X) & u8(Pawn::Collision))
#define pawn_is_box_or_block(X)  bool(u8(X) & u8(Pawn::Box_Or_Block))
#define pawn_is_mover(X)         bool(u8(X) & u8(Pawn::Mover))
#define pawn_is_block(X)         bool(u8(X) & u8(Pawn::Block))
#define pawn_is_box(X)           bool(u8(X) & u8(Pawn::Box))
#define pawn_is_goal(X)          bool(u8(X) & u8(Pawn::Goal))
#define pawn_is_pusher(X)        bool(u8(X) & u8(Pawn::Pusher))
#define pawn_is_empty(X)         bool(u8(X) == 0)


enum struct Direction {
	Up = 0,
	Right,
	Down,
	Left,
};
/*
	Grid/Sokoban Game

	origin -------> x-axis
	|
	| (0,0) (1,0) ...
	|
	| (0,1) (1,1) ...
	v
    y_axis

*/

struct Grid {	
	Pawn *data;
	i32 width; // also stride for indexing
	i32 height;
	
	bool pawn_move(i32 x, i32 y, Direction direction);
	bool pawn_move(i32 x, i32 y, Vector2i direction);
	bool can_move(i32 x, i32 y, Vector2i direction);
	bool _could_move(i32 x, i32 y, Vector2i direction);

	force_inline bool in_grid(i32 x, i32 y) {
		return 0<=x && x<width && 0<=y && y<height;
	}
	
	bool is_solved();

	
	Vector2i get_pusher_position();
	
	force_inline void swap_top_layer(i32 a, i32 b) {
		Pawn first = get(a);
		Pawn second = get(b);
		set(a, Pawn(top_layer(second) | bot_layer(first)));
		set(b, Pawn(top_layer(first)  | bot_layer(second)));

	}
	force_inline isize get_count() const {
		return width * height;
	}
	force_inline i32 as_index(i32 x, i32 y) {
		return width*y + x;
	}
	force_inline i32 as_index(const Vector2i &v) {
		return width*v.y + v.x;
	}	
	static force_inline i32 grid_as_index(const Vector2i &size, const Vector2i &v) {
		return size.x*v.y + v.x;
	}	
	force_inline Vector2i as_tile(i32 index) {
		i32 y = index/width;
        i32 x = index - y*width;
        return {x, y};
	}		
	force_inline Pawn get(i32 x, i32 y) {
		i32 index = width*y + x;
		assert(0<=x && x<width && 0<=y && y<height);
		assert(0<=index && index<get_count());
		return data[index];	
	}
	force_inline Pawn get(const Vector2i &v) {
		i32 index = width*v.y + v.x;
		assert(0<=v.x && v.x<width && 0<=v.y && v.y<height);
		assert(0<=index && index<get_count());
		return data[index];	
	}
	force_inline void set(i32 x, i32 y, Pawn pawn) {
		i32 index = width*y + x;
		assert(0<=x && x<width && 0<=y && y<height);
		assert(0<=index && index<get_count());
		data[index] = pawn;	
	}
	force_inline void set(i32 index, Pawn pawn) {
		assert(0<=index && index<get_count());
		data[index] = pawn;	
	}
	
	force_inline Pawn get(i32 index) {
		assert(0<=index && index<get_count());
		return data[index];	
	}
	force_inline Pawn &operator()(i32 x, i32 y) {
		i32 index = width*y + x;
		assert(0<=x && x<width && 0<=y && y<height);
		assert(0<=index && index<get_count());
		return data[index];
	}
	force_inline const Pawn &operator()(i32 x, i32 y) const {
		i32 index = width*y + x;
		assert(0<=x && x<width && 0<=y && y<height);
		assert(0<=index && index<get_count());
		return data[index];
	}
	void destroy();
};


Grid clone_grid(Grid &base);
String str(const Grid &grid);
std::ostream &operator<<(std::ostream &, const Grid &);
Grid make_grid(i32 width, i32 height);
void grid_remove_goals_and_pusher(Grid &);
i32 grid_block_count(Grid &);
struct Level {
	Grid grid;
	// i32 simulation_count;
	i32 box_count;
	f32 score;
	Chrono_Clock time_stamp; // time_stamp for tracking purposes
	Level clone();
};

Level make_level(Grid &, i32, f64, Chrono_Clock = Chrono_Clock());
bool level_less(Level *, Level *);

// simple for loop through the grid in order y, x with a Vector2i as iterator
#define for_grid(v, g) for(v.y=0; v.y<g.height; v.y+=1) for(v.x=0; v.x<g.width; v.x+=1)

#endif // SOKOBAN_H
#include "sokoban.h"


// Not being used for level generation, hence it is nothing special
bool Grid::is_solved() {
	auto count = get_count();
	for_range(i, 0, count) {
		u8 f = u8(data[i]);
		if( pawn_is_box(f) && !(f & u8(Pawn::Goal))) {
			return false;
		} 
	}
	return true;
}
bool Grid::pawn_move(i32 x, i32 y, Direction direction) {
								//   up        right   down    left
	const Vector2i directions_v[4] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
	return pawn_move(x, y, directions_v[u8(direction)]);
}

Vector2i Grid::get_pusher_position() {
	for_range(y, 0, height) {
		for_range(x, 0, width) {
			if(u8(get(x, y)) & u8(Pawn::Pusher)) {
				return {(i32)x, (i32)y};
			}
		}
	}
	crash("grid has no pusher");
	return Vector2i{-1, -1};
}
/*
	Tries to move the pawn at (x,y) in direction.
	Returns true if the the pawn actually moved else false.
	max recursion depth is 1
	Note:
		This function is *not* being used for level generation.
*/

bool Grid::pawn_move(i32 x, i32 y, Vector2i direction) {
	assert(abs(direction.x) != abs(direction.y));
	Vector2i d  = direction;
	Vector2i to = {x+direction.x, y+direction.y};

	if (!in_grid(to.x, to.y)) {
		return false;
	}
	u8 self = (u8)get(x, y);
	u8 other = (u8)get(to.x, to.y);
	assert( (top_layer(self) & u8(Pawn::Pusher)) || (top_layer(self) & u8(Pawn::Box)));
	// unpushable
	if(other == u8(Pawn::Block)) {
		return false;
	}
	assert(pawn_is_mover(self));
	
	// simple move
	// self in {Pusher, Box}
	if (!pawn_has_collision(other)) {
		// keep our bottom layer
		set(x, y, Pawn(bot_layer(self)));
		// moving our top layer and keeping the other bot layer
		set(to.x, to.y, Pawn(top_layer(self) | bot_layer(other)));
		
		return true;
	}
	// only pusher is allowed to push
	else if(self & u8(Pawn::Pusher)) {
		// must be a box
		assert(top_layer(other) == u8(Pawn::Box));
		
		bool moved = pawn_move(to.x, to.y, direction);
		if (moved) {
			u8 other = (u8)get(to.x, to.y);
			set(to.x, to.y, Pawn(top_layer(self) | bot_layer(other)));
			set(x, y, Pawn(bot_layer(self)));
			return true;
		}
		// failed to push so we don't move
		return false;
	}

	return false;
}
bool Grid::can_move(i32 x, i32 y, Vector2i direction) {
	assert(abs(direction.x) != abs(direction.y));
	Vector2i d  = direction;
	Vector2i to = {x+direction.x, y+direction.y};

	if (!in_grid(to.x, to.y)) {
		return false;
	}
	u8 self = (u8)get(x, y);
	u8 other = (u8)get(to.x, to.y);
	//assert( (top_layer(self) & u8(Pawn::Pusher)) || (top_layer(self) & u8(Pawn::Box)));
	// unpushable
	if(other == u8(Pawn::Block)) {
		return false;
	}
	
	// simple move
	// self in {Pusher, Box}
	if (!pawn_has_collision(other)) {
		return true;
	}
	// only pusher is allowed to push
	else { //if(self & u8(Pawn::Pusher)) {
		// must be a box
		assert(top_layer(other) == u8(Pawn::Box));
		Vector2i push_to = to + direction;
		if(in_grid(push_to.x, push_to.y) && !pawn_has_collision(get(push_to.x, push_to.y))) {
			return true;
		}
		return false;
	}
	return false;
}

// this functions treats (x, y) as a pusher (even if it isn't)
// returns true if move from (x,y) in direction is possible
bool Grid::_could_move(i32 x, i32 y, Vector2i direction) {
	assert(abs(direction.x) != abs(direction.y));
	Vector2i d  = direction;
	Vector2i to = {x+direction.x, y+direction.y};

	if (!in_grid(to.x, to.y)) {
		return false;
	}
	u8 self = (u8)Pawn::Pusher;
	u8 other = (u8)get(to.x, to.y);
	// unpushable
	if(other == u8(Pawn::Block)) {
		return false;
	}
	
	// simple move
	if (!(other & (u8)Pawn::Box)) {
		return true;
	}
	else {
		// must be a box
		assert(top_layer(other) == u8(Pawn::Box));
		Vector2i push_to = to + direction;
		if(in_grid(push_to.x, push_to.y) && !((u8)get(push_to) & (u8)Pawn::Box_Or_Block)) {
			assert(!pawn_is_box(get(push_to)));
			return true;
		}
		return false;
	}
	return false;
}

void Grid::destroy() {
	mem_free(data);
}
Grid make_grid(i32 width, i32 height) {
	Grid grid;
	grid.width = width;
	grid.height = height;
	grid.data = make_zeroed_array<Pawn>(width*height);
	return grid;
}
Grid clone_grid(Grid &base) {
	Grid grid;
	grid.width = base.width;
	grid.height = base.height;
	grid.data = clone_array(base.data, base.get_count());
	return grid;
}

void grid_remove_goals_and_pusher(Grid &grid) {
	Vector2i tile;
	for_grid(tile, grid) {
		auto idx = grid.as_index(tile);
		Pawn pawn = (Pawn)top_layer(grid.get(idx));
		if(pawn_is_pusher(pawn)) {
			grid.set(idx, Pawn::Empty);
		} else {
			grid.set(idx, pawn);
		}

	}
}
i32 grid_block_count(Grid &grid) {
	i32 count = 0;
	auto grid_count = grid.get_count();
	for_range(i, 0, grid_count) {
		auto it = grid.get(i);
		if(pawn_is_block(it)) {
			count += 1;
		}
	}
	return count;
}
bool level_less(Level *a, Level *b) {
    return a->score < b->score;
}
char pawn_to_char(Pawn pawn) {
	char pawn_to_char[] = {'p', 'x', 'c', '-', 'g', 'C', 'P'};
	switch(pawn) {
		case Pawn::Empty: return '-';
		case Pawn::Pusher: return 'p';
		case Pawn::Block: return 'x';
		case Pawn::Box: return 'c';
		case Pawn::Pusher_On_Goal: return 'P';
		case Pawn::Box_On_Goal: return 'C';
		case Pawn::Goal: return 'g';
		default:
		crash("unknown pawn to char");
	}
	return 0;
}
String str(const Grid &grid) {
	auto arr = make_array<char>(0, grid.get_count() + grid.height + 1);
	for_range(y, 0, grid.height) {
		for_range(x, 0, grid.width) {
			arr.add(pawn_to_char(grid(x, y)));
		}
		arr.add('\n');
	}
	return make_string(arr);
}


std::ostream &operator<<(std::ostream &os, const Grid &grid) {
	auto s = str(grid);
	print(s);
	s.destroy();
	return os;
}


Level Level::clone() {
	Level l = *this;
	l.grid = clone_grid(this->grid);
	return l;
}
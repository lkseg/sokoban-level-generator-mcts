/*
    This File should only get included *once*.
    This File contains basic utilities for the gui and file handling
*/
#ifndef APP_H
#define APP_H

#include "sokoban.h"
#include "util.h"

struct Game;
struct Level_Set;

enum struct App_State {
	Default = 0,
	Playing,
};

struct App {
	Vector2i screen_size = {1920, 1080};
	u8 target_fps = 60;
	int monitor;
	App_State state = App_State::Default;
	int text_size = 40;

	Camera2D camera;
	Vector2 tile_size;
	Array<Texture> textures;

	static const isize GOAL_GROUND = 200;
};
App app;
i16 char_to_pawn_id[255];
struct Level_Set {
	Array<Level> data;
	isize point;
	bool borrowed = false;
	void set(const Array<Level> &_data) {
		data = _data;
		point = data.count - 1;
		borrowed = true;
	}
	void add_simple(Grid &grid) {
		point = 0;
		data.add(make_level(grid, 0, 0));
	}
	void add_simple(Grid &&grid) {
		add_simple(grid);
	}
	Level &get_current_level() {
		return data[point];
	}
};
struct Game {

	Grid start_grid; // start configuration of the game grid
	Grid game_grid;  // grid we actively play on
	Level_Set level_set;
	isize level = 0;
	void set_level(isize idx) {
		level = idx;
		game_grid.destroy();
		start_grid.destroy();
		game_grid = clone_grid(level_set.data[idx].grid);
		start_grid = clone_grid(game_grid);
	}
	void reset_game_grid() {
		game_grid.destroy();
		game_grid = clone_grid(start_grid);
	}
	void set_next_level() {
		level = min(level+1, level_set.data.count-1);
		set_level(level);
	}
	void set_prev_level() {
		level = max<isize>(level-1, 0);
		set_level(level);
	}
	f32 get_current_score() {
		return level_set.data[level].score;
	}	
	void destroy() {
		game_grid.destroy();
		start_grid.destroy();
		if(!level_set.borrowed) {
			for_range(i, 0, level_set.data.count) {
				level_set.data[i].grid.destroy();
			}
		}
		level_set.data.destroy();
	}
};
#ifndef NO_GUI
Vector2i get_input_direction() {
	Vector2i d = {0, 0};
	if (IsKeyDown(KEY_W)) d.y -= 1;
	if (IsKeyDown(KEY_D)) d.x += 1;
	if (IsKeyDown(KEY_S)) d.y += 1;
	if (IsKeyDown(KEY_A)) d.x -= 1;
	return d;
}
Vector2i get_input_direction_once() {
	Vector2i d = {0, 0};
	if (IsKeyPressed(KEY_W)) d.y -= 1;
	if (IsKeyPressed(KEY_D)) d.x += 1;
	if (IsKeyPressed(KEY_S)) d.y += 1;
	if (IsKeyPressed(KEY_A)) d.x -= 1;
  return d;
}
Vector2i get_one_input_direction() {
	if (IsKeyPressed(KEY_W)) {
		return {0, -1};
	}
	if (IsKeyPressed(KEY_D)) {
		return {1, 0};
	}
	if (IsKeyPressed(KEY_S)) {
		return {0, 1};
	}
	if (IsKeyPressed(KEY_A)) {
		return {-1, 0};
	}
	return {0,0};
}


struct String_Reader {
	String data;
	isize point;
	force_inline char get() {
		assert(point<data.count);
		return data[point];
	}
	force_inline void next() {
		point += 1;
	}
	force_inline bool done() {
		return point >= data.count;
	}
};
bool parse_next_match(String_Reader &r, String string) {
	while(!r.done()) {
		if(r.get() == string[0]) {
			for_range(i, 0, string.count) {
				if(string[i] == r.get()) {
					r.next();
					if(r.done()) {
						if(string.count-1 == i) return true;
						return false;
					}
				} else {
					return false;
				}
			}
			return true;
		}
		r.next();
	}
	return false;
}


int parse_next_int(String_Reader &r) {
	isize start = -1;
	isize count = -1;
	bool first_digit = false;
	while(!r.done()) {
		char c = r.get();
		if(!isdigit(c) && first_digit) {
			count = r.point - start;
			break;
		}
		if(!first_digit) {
			start = r.point;
			first_digit = true;
		}
		r.next();
	}
	assert(start >= 0);
	char buf[255];
	for_range(i, 0, count) buf[i] = r.data[start+i];
	buf[count] = '\0';
	auto val = atoi(buf);
	return val;
}


String load_file_data(const char *file_name) {
	String string;
	unsigned int read;
	string.data = (const char*)LoadFileData(file_name, &read);
	release_assert(string.data != nullptr, "couldn't find/load file make sure it is in ./saved_levels. See README");
	string.count = read;
	return string;
}
Array<Grid> parse_file_data(const char *file_name, isize count = -1) {
	String data = load_file_data(file_name);
	Array<Grid> grids;
	if(count < 0) {
		count = 0;
		auto reader = String_Reader{data, 0};
		while(parse_next_match(reader, "LEVEL")) {
			count += 1;
		}
	}
	auto reader = String_Reader{data, 0};	
	for_range(_level, 0, count) {
		bool read_ok = parse_next_match(reader, "LEVEL");
		assert(read_ok);
		i32 size_x = parse_next_int(reader);
		i32 size_y = parse_next_int(reader);
		assert(size_x>0 && size_y>0);
		Grid grid = make_grid(size_x, size_y);
		for_range(y, 0, size_y) {
			while(char_to_pawn_id[reader.get()] <0) reader.next();
			assert(!reader.done());
			for_range(x, 0, size_x) {
				char c = reader.get();
				auto pawn_id = char_to_pawn_id[c];
				assert(pawn_id>=0);
				Pawn pawn = Pawn(pawn_id);
				grid(x, y) = pawn;
				reader.next();
			}
		}
		grids.add(grid);
	}
	data.destroy();
	return grids;
}
void save_level_set(Level_Set &set, u64 seed) {
	
	Array<char> arr;
	char buffer[2048];
	for_range(i, 0, set.data.count) {
		auto &level = set.data[i];
		arr.add('L');
		arr.add('E');
		arr.add('V');
		arr.add('E');
		arr.add('L');
		arr.add(' ');
		sprintf(buffer, "%d", level.grid.width);
		for(isize j=0; buffer[j] != '\0'; j+=1) {
			arr.add(buffer[j]);
		}
		arr.add(' ');
		sprintf(buffer, "%d", level.grid.height);
		for(isize j=0; buffer[j] != '\0'; j+=1) {
			arr.add(buffer[j]);
		}
		arr.add('\n');

		
		String s = str(level.grid);
		for_range(i, 0, s.count) {
			arr.add(s[i]);
		}
		s.destroy();
		arr.add('\n');
		arr.add('\n');
	}
	arr.add('\0');
	sprintf(buffer, "saved_levels/%ld.txt", seed);
	SaveFileData(buffer, &arr[0], arr.count-1);
	println("SAVED LEVEL SET");
	arr.destroy();
}
#else
Array<Grid> parse_file_data(const char *file_name, isize count = -1) {
	return {};
}
#endif // NO_GUI
void make_level_set_from(Game &game, String file_name) {
	println("loading" ,file_name);
	auto name = concat("saved_levels/", file_name);
	auto grids = parse_file_data(name.data);
	for_range(i, 0, grids.count) {
		game.level_set.add_simple(grids[i]);
		grids[i].destroy();
	}
	grids.destroy();
	name.destroy();
}
// #ifdef XTRACK
struct Tracking_Data {
	isize index;
	f64 duration;
	usize allocated; // kb
	u32 rollout_depth;
	f64 score;
};
void save_tracking_data(Array<Tracking_Data> &data) {

	FILE *file = fopen("data_track/data.txt", "w");
	for_range(i, 0, data.count) {
		auto &it = data[i];
		fprintf(file, "{%ld, %2.8f, %ld, %d, %2.8f}\n", it.index, it.duration, it.allocated, it.rollout_depth, it.score);
	}
	fclose(file);
}
struct Score_Tracking_Data {
	f64 time;
	f64 score;	
};
void save_score_tracking_data(Array<Score_Tracking_Data> &data) {

	FILE *file = fopen("data_track/score_data.txt", "w");
	for_range(i, 0, data.count) {
		auto &it = data[i];
		fprintf(file, "{%2.8f, %2.8f}\n", it.time, it.score);
	}
	fclose(file);
}
// #endif


#endif // APP_H
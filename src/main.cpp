#include "raylib.h"

#include "util.h"
#include "sokoban.h"
#include "mcts.h"
#include "malloc.h"
#include "allocator.h"
#include "experiment.h" // Includes example_levels and app


Game game = {};

void init_basic();
void init_raylib();
void uct_tests();
String scan_args(char **, int);
void print_random_seeds(int);
Mcts *make_custom_level();

int main(int arg_count, char **args) {
	print_and_check_settings();	
	// uct_tests(); return 0;	
	Default_Allocator default_allocator;
	global_default_allocator = &default_allocator;	
	global_allocator = global_default_allocator;

	#if ARENA_ALLOCATOR
	auto arena_allocator = make_arena_allocator(global_default_allocator);
	global_arena_allocator = &arena_allocator;
	#endif // ARENA_ALLOCATOR

	auto arg_string = scan_args(args, arg_count);
	init_basic();

	// CHECK FOR MEMORY LEAKS
	// Helper for checking memory leaks/bad access in a fast manner.
	if constexpr(false) {
		init_experiment();
		run_experiment_mcts_timeout_bootstrap(DEFAULT_SEED, node_ucb1_tuned, 3.0);
		run_experiment_mcts_timeout(DEFAULT_SEED, node_ucb1_tuned, 3.0);
		
		free_globals();
		println("all good");
		arg_string.destroy();
		return 0;		
	}

	if constexpr(EXPERIMENTS) {
		// produce some random seeds for static experiments
		if constexpr(false) {
			print_random_seeds(10);
			return 0;
		}		
		
		init_experiment();
		// experiment_box_cutoff();
		// experiment_depth_soft_cutoff();
		// experiment_action_delete_obstacle();
		// experiment_depth_cutoff();
		// experiment_mcts_bootstrap();
		// experiment_ucb1_constant();
		// experiment_uct_enhancements();
		// experiment_ucb1_constant();
		// experiment_mcts_bootstrap();
		// experiment_configuration();
		// experiment_uct_enhancements();
		// experiment_arena_allocator();
		// experiment_configuration();

		// run_with_tracking_no_bt(DEFAULT_SEED);		
		// run_with_tracking(DEFAULT_SEED);
		free_globals();
		arg_string.destroy();
		return 0;
	}
	// Deriving the parameters
	if constexpr(false) {
		test_level_set_derive_values(game);
		return 0;	
	}	

	

	Mcts *mcts;
	mcts = new_mcts(DEFAULT_SEED, DEFAULT_BOARD_SIZE, DEFAULT_START_POSITION);
	const i32 SIM_COUNT = SIMULATION_COUNT;
	
	const isize TO_KB = 1000;
	const isize TO_MB = 1000*TO_KB;

	
	if(!USE_EXAMPLE_LEVELS && arg_string.is_empty()) {
		
		
		println("Using Seed", mcts->seed);		
		mcts->start();

		// The decision procedure that is being used in the tree_policy.
		Decision_Proc decision_proc = node_ucb1_tuned;
		Chrono_Clock point_start;
		// We have to use preprocessors because msvc(windows) doesn't have mallinfo2.
		#if TRACK_DATA == false 
		{
			point_start = get_time();
			// no timeout
			if(DEFAULT_TIMEOUT == 0) {
				#if MCTS_BOOTSTRAP == true
				crash("Bootstrapping is not available without a timeout.\n Make sure to set the Timeout in settings.h");
				#else 
				run_mcts_rollout_count(mcts, decision_proc, SIM_COUNT);
				#endif // MCTS_BOOTSTRAP
			} else {
				#if MCTS_BOOTSTRAP == true
				auto counter = run_mcts_timeout_and_bootstrap(&mcts, decision_proc, DEFAULT_TIMEOUT, true, true, true);
				#else
				auto counter = run_mcts_timeout(mcts, decision_proc, DEFAULT_TIMEOUT);
				#endif // MCTS_BOOTSTRAP
				println("Simulation Count: ", counter);
				
			}
		} 
		#else
		{
			// This part is used for tracking the data usage for the paper
			// See the part before this for a normal process.
			println("track data");
			
			Array<Tracking_Data> tracking_data;
			tracking_data.resize(SIM_COUNT);

			point_start = get_time();
			for_range(i, 0, (isize)SIM_COUNT) {			
				auto point_start = get_time();
				mcts->next_rollout(decision_proc);
				auto duration = time_diff(point_start, get_time());

				auto info = mallinfo2();			
				tracking_data[i] = Tracking_Data{i, duration, info.uordblks/TO_KB, mcts->last_rollout_depth, mcts->best_score};
			}
			save_tracking_data(tracking_data);
			// mallinfo only on linux
			auto info = mallinfo2();
			println("allocated:", info.uordblks/TO_MB, "mb | free:", info.fordblks/TO_MB, "mb");
		}
		#endif // TRACK_DATA	
		auto point_end = get_time();

		print_children(mcts->root);		
		println("allocated: ", get_malloc_allocation_size()/1000, "MB");
		println("mcts duration: ", time_diff(point_start, point_end));
		game.level_set.set(mcts->get_level_set(LEVEL_SET_SIZE));
		// assert(mcts->root->rollout_count == SIM_COUNT);
	} else {
		if(!arg_string.is_empty()) {
			make_level_set_from(game, arg_string);
			arg_string.destroy();
		} else {
			make_test_level_set(game);
		}
	}	
	
	game.set_level(game.level_set.data.count-1);
	println_str(game.game_grid);

	#ifndef NO_GUI
	// The following is just the gui part of the application.
	init_raylib();
	char string_buffer[256];
	app.state = App_State::Playing;
	Vector2 tile_size = app.tile_size;
	auto &camera = app.camera;
	auto &textures = app.textures;

	bool save_image = false;
	int save_count = 0;

	while (!WindowShouldClose()) {
		if(IsKeyPressed(KEY_K)) {
			save_level_set(game.level_set, mcts->seed);
		}
		if(IsKeyPressed(KEY_F11)) {
			ToggleFullscreen();
		}
		if(IsKeyPressed(KEY_ONE)) {
			game.set_prev_level();
			app.state = App_State::Playing;
		}
		
		else if(IsKeyPressed(KEY_TWO)) {
			game.set_next_level();
			app.state = App_State::Playing;
		}
		RenderTexture2D render;
		if(IsKeyPressed(KEY_P)) {
			save_image = true;
			save_count += 1;
			render = LoadRenderTexture(app.screen_size.x, app.screen_size.y);
			BeginTextureMode(render);
		}
		if(IsKeyPressed(KEY_SPACE)) {

		}
		if(app.state == App_State::Playing) {
			Vector2i pusher;
			if(IsKeyPressed(KEY_R)) {
				game.reset_game_grid();
				pusher = game.game_grid.get_pusher_position();
			} else {
				pusher = game.game_grid.get_pusher_position();
			}
			Vector2i input_d = get_one_input_direction();
			if( (input_d.x != input_d.y) && game.game_grid.pawn_move(pusher.x, pusher.y, input_d)) {
				pusher = pusher + input_d;
				if(game.game_grid.is_solved()) {
					app.state = App_State::Default;
				}
			}
		}
		app.screen_size = {GetScreenWidth(), GetScreenHeight()};
		camera.offset = vec2(app.screen_size/2);
		camera.zoom = clamp<f32>(camera.zoom + GetMouseWheelMove()*0.05, 0.1, 100);
		Vector2 screen = vec2(app.screen_size);
		Vector2 start = {0, 0};
		

		BeginDrawing();
		ClearBackground(WHITE);
		BeginMode2D(camera);
		Vector2 pos = start;
		Grid &grid = game.game_grid;
		camera.target = tile_size*Vector2{(f32)grid.width, (f32)grid.height}/2.f;
		// Draw ground
		for_range(y, 0, grid.height) {
			for_range(x, 0, grid.width) {
				Pawn pawn = grid(x,y);
				if(pawn_is_goal(pawn)) {
					DrawTextureV(textures[i32(Pawn::Empty)], pos, WHITE);
				} else {
					DrawTextureV(textures[i32(Pawn::Empty)], pos, WHITE);
				}
				pos.x += tile_size.x;
			}
			pos.x = start.x;
			pos.y += tile_size.y;			
		}
		pos = start;
		for_range(y, 0, grid.height) {
			for_range(x, 0, grid.width) {
				Pawn pawn = grid(x,y);
				if (pawn != Pawn::Empty) {
					
					DrawTextureV(textures[i32(pawn)], pos, WHITE);
					if(pawn == Pawn::Pusher_On_Goal) {
						DrawTextureV(textures[i32(Pawn::Goal)], pos, WHITE);
					}
				}
				pos.x += tile_size.x;
			}
			pos.x = start.x;
			pos.y += tile_size.y;
		}
		
		EndMode2D();
		sprintf(string_buffer, "index: %ld, score: %2.4f", game.level, game.get_current_score());
		auto length = MeasureText(string_buffer, app.text_size);
		if(!save_image) {			
			DrawText(string_buffer, app.screen_size.x/2-length/2, 20, app.text_size, DARKGRAY);
		}
		EndDrawing();
		if(save_image) {
			save_image = false;
			auto image = LoadImageFromTexture(render.texture);
			// opengl stuff
			ImageFlipVertical(&image);
			auto off = grid.width*16*4;
			auto start = app.screen_size/2 - Vector2i{off, off};			
			Rectangle r = {(f32)start.x, (f32)start.y, (f32)off*2, (f32)off*2};
			ImageCrop(&image, r);
			char buffer[2048];
			sprintf(buffer, "%d.png", save_count);
			String file = concat("__images/", buffer);
			ExportImage(image, file.data);
			file.destroy();
			UnloadImage(image);
			UnloadRenderTexture(render);
		}
	}
	for_range(i, 0, textures.count) {
		if(textures[i].id != 0) {
			UnloadTexture(textures[i]);
		}
	}
	textures.destroy();
	CloseWindow();
	#endif // NO_GUI
	game.destroy();
	delete_mcts(mcts);
	
	#if ARENA_ALLOCATOR
	arena_allocator.destroy();
	#endif // ARENA_ALLOCATOR

	println("all good\n");
	return 0;
}

void print_and_check_settings() {
	// if this fails to compile it means someone removed '{}' from these settings
	auto size = Vector2i DEFAULT_BOARD_SIZE;
	auto start = Vector2i DEFAULT_START_POSITION;
	release_assert(size.x>=0 && size.y>=0 && 16<=size.x*size.y && size.x*size.y<=254, "bad level size: see settings.h!");
	if(start.x != -1) {
		release_assert(start.x>=0 && start.y>=0, "for the default start position set it to (-1, *): see settings.h!");
		release_assert(start.x < size.x && start.y < size.y, "start position must be inside the level");
	}
	release_assert(0.0 <= MCTS_BOOTSTRAP_DELTA && MCTS_BOOTSTRAP_DELTA <= 1.0, "bootstrap delta must be in [0,1]");
	release_assert(BOX_UPPER_CUTOFF != 0, "upper box cutoff can't be 0");
	release_assert(LEVEL_SET_SIZE > 0, "level set size must be greater than 0");
	println("box cutoff:", "[", BOX_LOWER_CUTOFF, BOX_UPPER_CUTOFF,"]");
	println("depth cutoff:", DEPTH_LOWER_CUTOFF);
	println("remove impossible:", REMOVE_IMPOSSIBLE);
	println("arena allocator:", ARENA_ALLOCATOR);
	println("bootstrap, count, delta:", MCTS_BOOTSTRAP, ",", MCTS_BOOTSTRAP_COUNT, ",", MCTS_BOOTSTRAP_DELTA);
	println("enhanced move agent:", !USE_SIMPLE_MOVES);
	println("level size", Vector2i DEFAULT_BOARD_SIZE);


}

void init_basic() {
	for_range(i, 0, 255) {
		char_to_pawn_id[i] = -1;
	}
	char_to_pawn_id['p'] = i16(Pawn::Pusher);
	char_to_pawn_id['x'] = i16(Pawn::Block);
	char_to_pawn_id['c'] = i16(Pawn::Box);   // c ^= Crate
	char_to_pawn_id['-'] = i16(Pawn::Empty);
	char_to_pawn_id['g'] = i16(Pawn::Goal);
	char_to_pawn_id['C'] = i16(Pawn::Box_On_Goal);
	char_to_pawn_id['P'] = i16(Pawn::Pusher_On_Goal);
}
#ifndef NO_GUI
void init_raylib() {

	SetTraceLogLevel(LOG_WARNING);	
	InitWindow(app.screen_size.x, app.screen_size.y, "Sokoban Sim");
	SetTargetFPS(app.target_fps);
	// GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
	app.monitor = GetCurrentMonitor();
	app.screen_size = {i32(GetMonitorWidth(app.monitor)*0.8), i32(GetMonitorHeight(app.monitor)*0.8)};
	SetWindowSize(app.screen_size.x, app.screen_size.y);
	SetWindowState(FLAG_WINDOW_RESIZABLE);

	// @fragile
	// SetWindowPosition(50, 50);
	
	Array<Texture> textures = make_array<Texture>((isize)256);
	for_range(i, 0, textures.count) {
		textures[i].id = 0;
	}
    textures[(isize)Pawn::Empty]          = LoadTexture("textures/empty.png");
	textures[(isize)Pawn::Pusher]         = LoadTexture("textures/pusher.png");
	textures[(isize)Pawn::Block]          = LoadTexture("textures/block.png");
	textures[(isize)Pawn::Box]            = LoadTexture("textures/box.png");
	textures[(isize)Pawn::Goal]           = LoadTexture("textures/goal.png");
	textures[(isize)Pawn::Box_On_Goal]    = LoadTexture("textures/box_on_goal.png");
	textures[(isize)Pawn::Pusher_On_Goal] = LoadTexture("textures/pusher.png");


	Vector2 offset = Vector2{(f32)textures[0].width, (f32)textures[0].height};
	auto camera = Camera2D{};
	camera.zoom = 2.f;
	camera.target = {0, 0};
	camera.offset = vec2(app.screen_size/2);

	app.camera = camera;
	app.tile_size = offset;
	app.textures = textures;
}
#endif // NO_GUI
String scan_args(char **args, int count) {
	// arg0 is prog name
	if(count <= 1) return {};

	if(count >= 3) {
		char *load = *(args+1);
		if(!(String(load) == String("load"))) {
			println("missing 'load' if loading a file was intended");
			return {};
		}
		char *arg = *(args+2);

		// sscanf(arg, "%d", &arg_val);
		auto s = concat(arg, ".txt");
		return s;
	}
	return String{};
}
void uct_tests() {	
	f64 data[] = {0.8, 0.1, 0.5};
	UCB1_C = 1.0/SQRT2;
	// f64 data[] = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1};
	const isize count = sizeof(data)/sizeof(f64);
	f64 sum = 0, sq_sum = 0;
	for_range(i, 0, (isize)count) {
		sum += data[i];
		sq_sum += data[i]*data[i];
	}
	auto mean = sum/(f64)count;
	f64 variance = 0;
	for_range(i, 0, (isize) count) {
		auto a = (mean-data[i]);
		variance += a * a;
	}
	variance /= (count-1);
	i64 total = 2*count+1;
	i64 local = count;
	auto direct_variance = (sq_sum - (sum*sum)/local)/(local-1);
	// these are old
	println(mean, variance, direct_variance);
	println(sum, sq_sum, total, local);
	println(ucb1(sum, total, local), "~=", 2.0775);
	println(ucb1_tuned(sum, sq_sum, total, local), "~=", 0.8694);
	println(ucb_v(sum, sq_sum, total, local), "~=", 3,2017);
	println(sp_mcts(sum, sq_sum, total, local), "~=", 2.8487);
	println("---------------------------------");
	for_range(t, local, total) {
		println(t, ":", ucb1(sum, t, local));
	}
	println("---------------------------------");
	for_range(t, local+1, total) {
		println(ucb1_tuned(sum, sq_sum, t, local));
	}
	println("---------------------------------");
	for_range(t, local+1, total) {
		println(ucb_v(sum, sq_sum, t, local));
	}
	println("---------------------------------");
	for_range(t, local+1, total) {
		println(sp_mcts(sum, sq_sum, t, local));
	}
	
}
void print_random_seeds(int num) {
		init_experiment();
		print("\n");
		for_range(i, 0, num) {
			print(randi_range(0, I32_MAX), ",");
		}
}

// This was meant as a proof of concept for using this level generator as a way
// to fill custom levels.
// It generally works but requires to disable the delete operation because you want to
// keep your custom level intact.
// Which is pretty problematic since the scoring method and post processing
// kind of depend on deletion being a thing.
Mcts *make_custom_level() {
	auto grids = parse_file_data("example_levels/custom_level.txt", 1);
	auto grid = grids[0];
	auto pos = grid.get_pusher_position();
	auto mcts = new_mcts_bootstrap(DEFAULT_SEED, {grid.width, grid.height}, pos);
	// mcts->no_delete = true;
	mcts->box_upper_cutoff = grid.width*grid.height/3.0;
	root_add_custom_child(mcts, grid, 0);
	destroy_inner(grids);
	grids.destroy();
	run_mcts_timeout(mcts, node_ucb1_tuned, DEFAULT_TIMEOUT);
	return mcts;
}
#ifndef SOKOBAN_COMPARISON_LEVELS
#define SOKOBAN_COMPARISON_LEVELS
/*
  This File should only get included *once*  
*/
#include "mcts.h"
#include "mcts_actions.h"
#include "util.h"
#include "app.h"

void test_mark_goal(Mcts_Node &node, Vector2i box, Vector2i goal) {
	auto boxi = node.grid.as_index(box.x, box.y);
	auto goali = node.grid.as_index(goal.x, goal.y);
	assert(pawn_is_box(node.grid.get(box.x, box.y)));
	assert(pawn_is_goal(node.grid.get(goal.x, goal.y)));
	assert(node.second[boxi] == INVALID_INDEX);
	node.second[boxi] = goali;
}
Mcts_Node test_node_ex_1(Grid &grid) {
	Mcts_Node node;
	node.box_count = 1;
	node.grid = grid;
	node.second = make_array<u8>(25);
	for_range(i, 0, node.second.count) {
		node.second[i] = INVALID_INDEX;
	}
	test_mark_goal(node, {3,2}, {3,4});
	node.flags |= MCTS_TERMINAL;
	node.pusher = grid.as_index(grid.get_pusher_position());
	return node;
}
Mcts_Node test_node_ex_2(Grid &grid) {
	Mcts_Node node;
	node.box_count = 3;
	node.grid = grid;
	node.second = make_array<u8>(25);
	for_range(i, 0, node.second.count) {
		node.second[i] = INVALID_INDEX;
	}
	test_mark_goal(node, {1,2}, {0,1});
	test_mark_goal(node, {3,2}, {4,3});
	test_mark_goal(node, {2,3}, {4,2});
	node.flags |= MCTS_TERMINAL;
	node.pusher = grid.as_index(grid.get_pusher_position());
	return node;
}
Mcts_Node test_node_ex_3(Grid &grid) {
	Mcts_Node node;
	node.box_count = 4;
	node.grid = grid;
	node.second = make_array<u8>(25);
	for_range(i, 0, node.second.count) {
		node.second[i] = INVALID_INDEX;
	}
	test_mark_goal(node, {1,0}, {3,0});
	test_mark_goal(node, {2,1}, {2,4});
	test_mark_goal(node, {1,2}, {0,4});
	test_mark_goal(node, {1,3}, {0,3});
	node.flags |= MCTS_TERMINAL;
	node.pusher = grid.as_index(grid.get_pusher_position());
	return node;
}
Mcts_Node test_node_ex_4(Grid &grid) {
	Mcts_Node node;
	node.box_count = 5;
	node.grid = grid;
	node.second = make_array<u8>(25);
	for_range(i, 0, node.second.count) {
		node.second[i] = INVALID_INDEX;
	}
	test_mark_goal(node, {1,1}, {0,0});
	test_mark_goal(node, {2,1}, {1,0});
	test_mark_goal(node, {1,2}, {1,4});
	test_mark_goal(node, {3,2}, {3,0});
	test_mark_goal(node, {2,3}, {4,3});
	node.flags |= MCTS_TERMINAL;
	node.pusher = grid.as_index(grid.get_pusher_position());
	return node;
}
Mcts_Node test_node_ex_5(Grid &grid) {
	Mcts_Node node;
	node.box_count = 6;
	node.grid = grid;
	node.second = make_array<u8>(25);
	for_range(i, 0, node.second.count) {
		node.second[i] = INVALID_INDEX;
	}
	test_mark_goal(node, {0,1}, {0,3});
	test_mark_goal(node, {1,1}, {1,3});
	test_mark_goal(node, {2,1}, {3,0});
	test_mark_goal(node, {1,2}, {4,2});
	test_mark_goal(node, {3,2}, {4,1});
	test_mark_goal(node, {2,3}, {4,3});
	node.flags |= MCTS_TERMINAL;
	node.pusher = grid.as_index(grid.get_pusher_position());
	return node;
}
void make_test_level_set(Game &game) {
	auto grids = parse_file_data("example_levels/example_levels.txt", 5);
	game.level_set.add_simple(grids[0]);
	game.level_set.add_simple(grids[1]);
	game.level_set.add_simple(grids[2]);
	game.level_set.add_simple(grids[3]);
	game.level_set.add_simple(grids[4]);
	
	auto ex_node_1 = test_node_ex_1(grids[0]);
	auto ex_node_2 = test_node_ex_2(grids[1]);
	auto ex_node_3 = test_node_ex_3(grids[2]);
	auto ex_node_4 = test_node_ex_4(grids[3]);
	auto ex_node_5 = test_node_ex_5(grids[4]);
	Mcts mcts;
	mcts.start_position = grids[0].as_index(2,2);
	mcts.start_position_tile = {2,2};
	mcts.area = 5*5;
	mcts.score_scale = get_score_scale(&mcts);
	println(score_node(ex_node_1, &mcts), "~= 0.4");
	println(score_node(ex_node_2, &mcts), "~= 0.6");
	println(score_node(ex_node_3, &mcts), "~= 0.8");
	println(score_node(ex_node_4, &mcts), "~= 1.0");
	println(score_node(ex_node_5, &mcts), "~= 1.2");
	{
		auto &node = ex_node_1;
		auto moves = get_all_possible_moves(node.grid.as_tile(node.pusher), node.grid);

		/* print_2d(visited, 5, 5);
		print_move_infos(moves, node.grid); */
		moves.destroy();
		println("----------------------");
	}
	{
		auto &node = ex_node_2;
		auto moves = get_all_possible_moves(node.grid.as_tile(node.pusher), node.grid);

		/* print_2d(visited, 5, 5);
		print_move_infos(moves, node.grid); */
		moves.destroy();
		println("----------------------");
	}
	{
		auto &node = ex_node_3;
		auto moves = get_all_possible_moves(node.grid.as_tile(node.pusher), node.grid);

		/* print_2d(visited, 5, 5);
		print_move_infos(moves, node.grid); */
		moves.destroy();
		println("----------------------");
	}
}
struct Data_Set_Struct {
	f64 wb;
	f64 wc;
	f64 wn;
	f64 alpha;
	f64 beta;
	f64 gamma;
};

void test_level_set_derive_values(Game &game) {
	auto grids = parse_file_data("example_levels/example_levels.txt", 5);
	game.level_set.add_simple(grids[0]);
	game.level_set.add_simple(grids[1]);
	game.level_set.add_simple(grids[2]);
	game.level_set.add_simple(grids[3]);
	game.level_set.add_simple(grids[4]);
	
	auto ex_node_1 = test_node_ex_1(grids[0]);
	auto ex_node_2 = test_node_ex_2(grids[1]);
	auto ex_node_3 = test_node_ex_3(grids[2]);
	auto ex_node_4 = test_node_ex_4(grids[3]);
	auto ex_node_5 = test_node_ex_5(grids[4]);
	Mcts mcts;
	mcts.start_position = grids[0].as_index(2,2);
	mcts.start_position_tile = {2,2};
	mcts.area = 5*5;
	mcts.score_scale = get_score_scale(&mcts);

	Data_Set_Struct set = {0,0,0};
	f64 val = F64_MAX;
	f64 k = 50;
	f64 step = 0.1;
	f64 wstep = 1.0;
	const isize start_a = 1;
	for_range(ai, start_a, 20) for_range(bi, start_a, 20) for_range(ci, start_a, 20)
	for_range(wbi, 1, 20) for_range(wci, 1, 20) for_range(wni, 1, 20) {
		f64 wb = wstep * (f64)wbi, wc = wstep * (f64)wci, wn = wstep * (f64)wni;
		f64 alpha = step * f64(ai), beta = step * f64(bi), gamma = step * f64(ci);
		auto a = score_node_test(ex_node_1, &mcts, wb, wc, wn, k, alpha, beta, gamma);
		auto b = score_node_test(ex_node_2, &mcts, wb, wc, wn, k, alpha, beta, gamma);
		auto c = score_node_test(ex_node_3, &mcts, wb, wc, wn, k, alpha, beta, gamma);
		auto d = score_node_test(ex_node_4, &mcts, wb, wc, wn, k, alpha, beta, gamma);
		auto e = score_node_test(ex_node_5, &mcts, wb, wc, wn, k, alpha, beta, gamma);
		if(a<b && b<c && c<d && d<e) {
			const auto av = abs(0.4-a);
			const auto bv = abs(0.6-b);
			const auto cv = abs(0.8-c);
			const auto dv = abs(1.0-d);
			const auto ev = abs(1.2-e);
			// f64 new_val = av + bv + cv + dv + ev;			
			// f64 new_val = cv*cv + dv*dv + ev*ev;
			f64 new_val = av*av + bv*bv + cv*cv + dv*dv + ev*ev;
			if(new_val < val) {
				set = Data_Set_Struct{wb, wc, wn, alpha, beta, gamma};
				val = new_val;
			}
		}
	}
	println(set.wb, set.wc, set.wn, set.alpha, set.beta, set.gamma);	
	println(score_node_test(ex_node_1, &mcts, set.wb, set.wc, set.wn, k, set.alpha, set.beta, set.gamma), "~= 0.4");
	println(score_node_test(ex_node_2, &mcts, set.wb, set.wc, set.wn, k, set.alpha, set.beta, set.gamma), "~= 0.6");
	println(score_node_test(ex_node_3, &mcts, set.wb, set.wc, set.wn, k, set.alpha, set.beta, set.gamma), "~= 0.8");
	println(score_node_test(ex_node_4, &mcts, set.wb, set.wc, set.wn, k, set.alpha, set.beta, set.gamma), "~= 1.0");
	println(score_node_test(ex_node_5, &mcts, set.wb, set.wc, set.wn, k, set.alpha, set.beta, set.gamma), "~= 1.2");
}


#endif // SOKOBAN_COMPARISON_LEVELS
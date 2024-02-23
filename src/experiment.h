#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include "sokoban.h"
#include "mcts.h"
#include "sokoban_example_levels.h"
#include "allocator.h"
void print_and_check_settings();
/*
	All the experiments are in this File.
	Some experiment generate the random seeds during runtime.
	For other experiments the seeds are being randomly generated beforehand and manually inserted, since
	some toggling some settings is a bit more complicated. 	 
*/


template<bool extra_check = false>
i64 run_mcts_timeout(Mcts *mcts, const Decision_Proc decision_proc, const f64 timeout) {
	mcts->start();
	auto point_start = get_time();
    i64 counter = 0;
	Chrono_Clock point_end;
	while(true) {
		mcts->next_rollout(decision_proc);
        counter += 1;
		point_end = get_time();
		if constexpr(extra_check) {
			if(mcts->finish_early) {
				println("FINISH EARLY");
				break;
			}	
		}
		if(time_diff(point_start, point_end) >= timeout) {
			break;
		}
	}	
	return counter;
}

i64 run_mcts_timeout_and_bootstrap(Mcts **_mcts, const Decision_Proc decision_proc, const f64 timeout, bool delete_first = true, bool print_swap = true, bool add_old_levels = false) {
	f64 delta = 1.0 - MCTS_BOOTSTRAP_DELTA;
	const f64 bt_timeout = MCTS_BOOTSTRAP_DELTA * timeout;
	auto mcts = *_mcts;
	auto counter = run_mcts_timeout(mcts, decision_proc, delta * timeout);	
	isize index = -1;
	// just sort
	for_range(i, 0, mcts->finished_nodes.count) {
		if(approx((f32)mcts->best_score, mcts->finished_nodes[i].score)) {
			index = i;
			break;
		}
	}
	release_assert(index >= 0);
	auto string = str(mcts->finished_nodes[index].grid);
	if (print_swap) {
		println(string);
	}
	string.destroy();
	auto n_mcts = new_mcts_bootstrap(mcts->seed, mcts->size, mcts->start_position_tile);
	
	auto n = min<isize>(MCTS_BOOTSTRAP_COUNT, mcts->finished_nodes.count);
	for_range(i, 0, n) {
		auto idx = mcts->finished_nodes.count - 1 - i;
		root_add_custom_child(n_mcts, mcts->finished_nodes[idx].grid, mcts->finished_nodes[idx].score);
	}
	auto old_mcts = mcts;	
	
	mcts = n_mcts;
	// println("1:", counter, old_mcts->root->rollout_count+mcts->root->rollout_count);
	counter += run_mcts_timeout<true>(mcts, decision_proc, bt_timeout);
	if(add_old_levels) {
		for_range(i, 0, old_mcts->finished_nodes.count) {			
			mcts->finished_nodes.add(old_mcts->finished_nodes[i].clone());
		}
	}
	// println("2:", counter, old_mcts->root->rollout_count+mcts->root->rollout_count);
	if(delete_first) {
		delete_mcts(old_mcts);
	}
	
	*_mcts = mcts;
	
	return counter;
}

f64 run_mcts_rollout_count(Mcts *mcts, const Decision_Proc decision_proc, const i32 count) {
	mcts->start();
	auto point_start = get_time();
	Chrono_Clock point_end;
	for_range(i, 0, (isize)count){
		mcts->next_rollout(decision_proc);
	}	
	point_end = get_time();
	return time_diff(point_start, point_end);
}
u64 get_random_seed() {
	return randi_range(1, I32_MAX);
}
void set_random_seeds(u64 *ptr, usize count) {
	for_range(i, 0, count) {
		*(ptr + i) = get_random_seed();
	}
}
isize get_malloc_allocation_size() {
	#ifdef GCC
	auto info = mallinfo2();
	const isize TO_KB = 1000;
	const isize TO_MB = 1000*TO_KB;
	return info.uordblks/TO_KB;	
	#endif // GCC
	return 0;
}
Mcts *new_experiment_mcts(u64 seed) {
    return new_mcts(seed, DEFAULT_BOARD_SIZE, DEFAULT_START_POSITION);
}
struct Experiment_Info {    
    f64 best_score;
	f64 best_score_time;
	i32 rollout_count;
};
struct Experiment_Info_Pair {
	Experiment_Info first;
	Experiment_Info second;
};

Experiment_Info get_experiment_info(Mcts *mcts, Chrono_Clock start) {
	Experiment_Info info;
	info.rollout_count = mcts->root->rollout_count;
	info.best_score = mcts->best_score;
	bool found = false;
	for_range(i, 0, mcts->finished_nodes.count) {
		if(mcts->finished_nodes[i].score == (f32)mcts->best_score) {
			info.best_score_time = time_diff(start, mcts->finished_nodes[i].time_stamp);			
			found = true;
			break;
		}
	}
	if(!found) crash("best score not found");	
	return info;
}
Experiment_Info_Pair run_experiment_mcts_timeout_bootstrap(u64 seed, Decision_Proc decision, f64 timeout, Array<Score_Tracking_Data> *data = nullptr) {
    auto mcts = new_experiment_mcts(seed);
	auto start = get_time();
	auto mcts_b = mcts;
    run_mcts_timeout_and_bootstrap(&mcts_b, decision, timeout, false, false);
	if(data) {
		for_range(i, 0, mcts->finished_nodes.count) {
			f64 time = time_diff(mcts->time_start, mcts->finished_nodes[i].time_stamp);
			data->add(Score_Tracking_Data{time, mcts->finished_nodes[i].score});
		}
		for_range(i, 0, mcts_b->finished_nodes.count) {
			f64 time = time_diff(mcts_b->time_start, mcts_b->finished_nodes[i].time_stamp);
			data->add(Score_Tracking_Data{time, mcts_b->finished_nodes[i].score});
		}
	}	
	Experiment_Info_Pair infos;
	infos.first = get_experiment_info(mcts, start);
	infos.second = get_experiment_info(mcts_b, start);
	delete_mcts(mcts_b);
    delete_mcts(mcts);
	return infos;
}
Experiment_Info run_experiment_mcts_timeout(u64 seed, Decision_Proc decision, f64 timeout, Array<Score_Tracking_Data> *data = nullptr) {
    auto mcts = new_experiment_mcts(seed);
	auto start = get_time();
    run_mcts_timeout(mcts, decision, timeout);
	if(data) {
		for_range(i, 0, mcts->finished_nodes.count) {
			f64 time = time_diff(start, mcts->finished_nodes[i].time_stamp);
			data->add(Score_Tracking_Data{time, mcts->finished_nodes[i].score});
		}
	}	
	auto info = get_experiment_info(mcts, start);
    delete_mcts(mcts);
	return info;
}
Experiment_Info run_experiment_mcts_count(u64 seed, Decision_Proc decision, i64 count, Array<Score_Tracking_Data> *data = nullptr) {
    auto mcts = new_experiment_mcts(seed);
	auto start = get_time();
    run_mcts_rollout_count(mcts, decision, count);
	if(data) {
		for_range(i, 0, mcts->finished_nodes.count) {
			f64 time = time_diff(start, mcts->finished_nodes[i].time_stamp);
			data->add(Score_Tracking_Data{time, mcts->finished_nodes[i].score});
		}
	}
	auto info = get_experiment_info(mcts, start);
    delete_mcts(mcts);
	return info;
}

struct Data_Average {	
	f64 reward; // best reward
	f64 time; // at time

	f64 std_reward;
	f64 std_time;

	i32 rollout_count;

};
std::ostream &operator<<(std::ostream &os, const Data_Average &v) {
	os << "{" << v.reward << "," << v.time << "," << v.std_reward << "," << v.std_time << "," << v.rollout_count << "}";
	return os;
}
void print_averages(Array<Data_Average> &data) {
	for_range(i, 0, data.count) {
		println(i, data[i]);
	}
	print("\n");
}
void save_data_averages(Array<Data_Average> &data) {
	char buffer[2048];
	FILE *file = fopen("data_track/data_average.txt", "w");
	for_range(i, 0, data.count) {
		auto &it = data[i];
		fprintf(file, "{%d, %2.8f, %2.8f}\n", it.rollout_count, it.time, it.reward);
	}
	fclose(file);
}

Array<Data_Average> get_averages(Array<Array<Experiment_Info>> &data) {
	auto averages = make_array<Data_Average>(0, 100);
	for_range(j, 0, data.count) {
		f64 avrg = 0;
		f64 time = 0;
		i32 r_count = 0;
		for_range(i, 0, data[j].count) {
			avrg += data[j][i].best_score;
			time += data[j][i].best_score_time;
			r_count += data[j][i].rollout_count;
		}
		avrg    = avrg/f64(data[j].count);
		time    = time/f64(data[j].count);
		r_count = r_count/data[j].count;
		f64 std_reward = 0;
		f64 std_time = 0;
		for_range(i, 0, data[j].count) {
			{
				auto val = avrg - data[j][i].best_score;
				std_reward += val * val;
			}
			{
				auto val = time - data[j][i].best_score_time;
				std_time += val * val;
			}
		}
		auto n = data[j].count;
		// bessel's correction
		// n > 1 should always be the case
		std_reward = n > 1 ? sqrt(std_reward /(n-1))   : 0;
		std_time   = n > 1 ? sqrt(std_time   /(n-1))   : 0;
		auto da = Data_Average{avrg, time, std_reward, std_time, r_count};
		averages.add(da);
	}
	return averages;
}

void free_globals() {
	#if ARENA_ALLOCATOR
	global_arena_allocator->destroy();
	#endif // ARENA_ALLOCATOR
}

void init_experiment(unsigned int r_seed = 0) {
	// srand just for generating the seed for mersenne
	if(r_seed == 0) {
		srand(time(0));
	} else {
		srand(r_seed);
	}
	auto bsize = Vector2i DEFAULT_BOARD_SIZE;
	auto start_pos = Vector2i DEFAULT_START_POSITION;
	// normally
	// release_assert(bsize.x == 5 && bsize.y == 5, "experiment board size must be 5x5");
	release_assert(start_pos.x == -1, "experiment start position must be default i.e. {-1, *}");
	u64 seed = rand();
	println("experiment uses seed for seed generation:", seed);
	
	set_global_random_engine_seed(seed);
}
void experiment_uct_enhancements() {
	DEPTH_LOWER_CUTOFF = 10; BOX_UPPER_CUTOFF = 9;
	release_assert(!MCTS_BOOTSTRAP && REMOVE_IMPOSSIBLE && ARENA_ALLOCATOR && DEPTH_LOWER_CUTOFF == 10 &&  BOX_UPPER_CUTOFF == 9 && BOX_LOWER_CUTOFF == 1 && USE_SIMPLE_MOVES == false);
    const i32 ITER_COUNT = 30;
    f64 TIMEOUT = 30.0;
	auto data = make_array<Array<Experiment_Info>>(4);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT];
	release_assert(ITER_COUNT == carray_len(seeds));
	set_random_seeds(seeds, ITER_COUNT);
	println("ucb1_c:", UCB1_C);
    for_range(i, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[i];
		println("iteration:", i,", seed:", seed);
        data[0].add(run_experiment_mcts_timeout(seed, node_ucb1      , TIMEOUT));
		println("ucb1", data[0][i].rollout_count);
		
        data[1].add(run_experiment_mcts_timeout(seed, node_ucb1_tuned, TIMEOUT));
		println("ucb1_tuned", data[1][i].rollout_count);
		
        data[2].add(run_experiment_mcts_timeout(seed, node_ucb_v     , TIMEOUT));
		println("ucb_v", data[2][i].rollout_count);
		
        data[3].add(run_experiment_mcts_timeout(seed, node_sp_mcts   , TIMEOUT));
		println("sp_mcts", data[3][i].rollout_count);
    }
	auto avrg = get_averages(data);
	
	
	auto it = avrg[0];
	println("ucb1",       it.rollout_count, it.reward, it.time, it.std_reward, it.std_time);
	it = avrg[1];
	println("ucb1_tuned", it.rollout_count, it.reward, it.time, it.std_reward, it.std_time);
	it = avrg[2];
	println("ucb_v",      it.rollout_count, it.reward, it.time, it.std_reward, it.std_time);
	it = avrg[3];
	println("sp_mcts",    it.rollout_count, it.reward, it.time, it.std_reward, it.std_time);
	print("\n");
	destroy_data(data);
}
void experiment_ucb1_constant() {
	DEPTH_LOWER_CUTOFF = 10; BOX_UPPER_CUTOFF = 9;
	release_assert(!MCTS_BOOTSTRAP && REMOVE_IMPOSSIBLE && ARENA_ALLOCATOR && DEPTH_LOWER_CUTOFF == 10 &&  BOX_UPPER_CUTOFF == 9 && BOX_LOWER_CUTOFF == 1 && USE_SIMPLE_MOVES == false);
	// release_assert(BOX_LOWER_CUTOFF == 0 && BOX_UPPER_CUTOFF == INT16_MAX && !ARENA_ALLOCATOR);
	const i32 ITER_COUNT = 30;
	f64 TIMEOUT = 30.0;	
	const i32 PARAM_COUNT = 6;
	i32 param[PARAM_COUNT] = {1, 2, 3, 4, 5, 6};
    
	auto data = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT];
	release_assert(ITER_COUNT == carray_len(seeds));
	set_random_seeds(seeds, ITER_COUNT);
	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		println("iteration", iteration);
		for_range(i, 0, (i32)PARAM_COUNT) {
			UCB1_C = param[i] * 1.0/SQRT2;
			
			println("C:", UCB1_C, ", seed: ", seed);
			data[i].add(run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT));
		}
	}
	auto avrg = get_averages(data);
	print_averages(avrg);
}
void experiment_depth_cutoff() {
	UCB1_C = 1.0/SQRT2;
	BOX_LOWER_CUTOFF = 1;
	DEPTH_LOWER_CUTOFF = 0;
	BOX_UPPER_CUTOFF = 9;

	release_assert(BOX_UPPER_CUTOFF == 9 && BOX_LOWER_CUTOFF == 1 && !ARENA_ALLOCATOR && USE_SIMPLE_MOVES == false && !REMOVE_IMPOSSIBLE && UCB1_C == 1.0/SQRT2);
	const i32 ITER_COUNT = 10;
	f64 TIMEOUT = 30.0;	
	const i32 PARAM_COUNT = 5;
	i32 param[PARAM_COUNT] = {0, 5, 7, 10, 15};
    
	auto data = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT];
	release_assert(ITER_COUNT == carray_len(seeds));
	set_random_seeds(seeds, ITER_COUNT);
	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		println("iteration", iteration);
		for_range(i, 0, (i32)PARAM_COUNT) {
			DEPTH_LOWER_CUTOFF = param[i];
			
			println("depth_cutoff:", DEPTH_LOWER_CUTOFF, ", seed: ", seed);
			data[i].add(run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT));
		}
	}
	auto avrg = get_averages(data);
	print_averages(avrg);
}
void experiment_depth_soft_cutoff() {
	UCB1_C = 1.0/SQRT2;
	BOX_LOWER_CUTOFF = 1;
	DEPTH_LOWER_CUTOFF = 10;
	BOX_UPPER_CUTOFF = 9;

	release_assert(BOX_UPPER_CUTOFF == 9 && BOX_LOWER_CUTOFF == 1 && !ARENA_ALLOCATOR && USE_SIMPLE_MOVES == false && !REMOVE_IMPOSSIBLE && UCB1_C == 1.0/SQRT2);
	const i32 ITER_COUNT = 10;
	f64 TIMEOUT = 30.0;	
	
	i32 param[] = {0, 4, 6};
    const i32 PARAM_COUNT = carray_len(param);
	auto data = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT];
	release_assert(ITER_COUNT == carray_len(seeds));
	set_random_seeds(seeds, ITER_COUNT);
	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		println("iteration", iteration);
		for_range(i, 0, (i32)PARAM_COUNT) {
			DEPTH_SOFT_CUTOFF = param[i];
			
			println("depth_cutoff:", DEPTH_SOFT_CUTOFF, ", seed: ", seed);
			data[i].add(run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT));
		}
	}
	auto avrg = get_averages(data);
	print_averages(avrg);
}
void experiment_box_cutoff() {
	DEPTH_LOWER_CUTOFF = 0;
	BOX_LOWER_CUTOFF = 1;
	UCB1_C = 1.0/SQRT2;
	release_assert(DEPTH_LOWER_CUTOFF == 0 &&  BOX_LOWER_CUTOFF == 1 && !ARENA_ALLOCATOR && USE_SIMPLE_MOVES == false && !REMOVE_IMPOSSIBLE && UCB1_C == 1.0/SQRT2);
	
	const i32 ITER_COUNT = 10;
	f64 TIMEOUT = 30.0;	
	
	i32 param[] = {EXPERIMENT_INF, 20, 15, 10, 5};
    const i32 PARAM_COUNT = carray_len(param);
	auto data = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT];
	release_assert(ITER_COUNT == carray_len(seeds));
	set_random_seeds(seeds, ITER_COUNT);
	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		println("iteration", iteration);
		for_range(i, 0, (i32)PARAM_COUNT) {
			BOX_UPPER_CUTOFF = param[i];
			
			println("box_cutoff:", BOX_UPPER_CUTOFF, ", seed: ", seed);
			data[i].add(run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT));
		}
	}
	auto avrg = get_averages(data);
	print_averages(avrg);
}

void experiment_configuration() {
	UCB1_C = 1.0/SQRT2;
	BOX_LOWER_CUTOFF = 1;
	DEPTH_LOWER_CUTOFF = 10;
	BOX_UPPER_CUTOFF = 9;
	release_assert(DEPTH_LOWER_CUTOFF == 10 &&  BOX_UPPER_CUTOFF == 9 && BOX_LOWER_CUTOFF == 1 && !ARENA_ALLOCATOR && USE_SIMPLE_MOVES == false && UCB1_C == 1.0/SQRT2);
	
	const i32 ITER_COUNT = 10;
	const f64 TIMEOUT = 30.0;	
	const i32 PARAM_COUNT = 1;
	// MAX ^= infinity
	bool param[PARAM_COUNT] = {REMOVE_IMPOSSIBLE};
    
	auto data = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT] = {628129731 , 1588217283 , 13190087 , 440618325 , 1586577968 , 1084525048 , 1120993636 , 1674046725 , 1541621626 , 1599912913};	

	release_assert(ITER_COUNT == carray_len(seeds));
	println("enhanced", REMOVE_IMPOSSIBLE);

	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		println("iteration", iteration);
		data[0].add(run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT));
	}
	auto avrg = get_averages(data);
	print_averages(avrg);
}
void experiment_move_agent() {
	UCB1_C = 1.0/SQRT2;
	BOX_LOWER_CUTOFF = 0;
	DEPTH_LOWER_CUTOFF = 0;
	BOX_UPPER_CUTOFF = EXPERIMENT_INF;
	release_assert(BOX_UPPER_CUTOFF == EXPERIMENT_INF && BOX_LOWER_CUTOFF == 0 && !ARENA_ALLOCATOR && DEPTH_LOWER_CUTOFF == 0 && !REMOVE_IMPOSSIBLE && UCB1_C == 1.0/SQRT2);
	
	const i32 ITER_COUNT = 10;
	const f64 TIMEOUT = 30.0;	
	const i32 PARAM_COUNT = 1;
	// MAX ^= infinity
	bool param[PARAM_COUNT] = {USE_SIMPLE_MOVES};
    
	auto data = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT] = { 1794791989 , 18089998 , 1609526469 , 913904819 , 1927522519 , 609250387 , 901396981 , 494921135 , 1892664378 , 507390561};	

	release_assert(ITER_COUNT == carray_len(seeds));
	println("enhanced", !USE_SIMPLE_MOVES);

	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		println("iteration", iteration);
		data[0].add(run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT));
	}
	auto avrg = get_averages(data);
	print_averages(avrg);
}
void experiment_arena_allocator() {
	UCB1_C = 1.0/SQRT2;
	BOX_UPPER_CUTOFF = 9;
	DEPTH_LOWER_CUTOFF = 10;
	release_assert(REMOVE_IMPOSSIBLE && BOX_LOWER_CUTOFF == 1 && USE_SIMPLE_MOVES == false && UCB1_C == 1.0/SQRT2);
	release_assert(DEPTH_LOWER_CUTOFF == 10 &&  BOX_UPPER_CUTOFF == 9);
	const i32 ITER_COUNT = 10;
	const f64 TIMEOUT = 30.0;	
	const i32 PARAM_COUNT = 1;
	// MAX ^= infinity
	bool param[PARAM_COUNT] = {ARENA_ALLOCATOR};
    
	auto data = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT] = {823631836 , 1581158456 , 1256722227 , 77992510 , 1959581929 , 1985534584 , 777972026 , 773604831 , 102885711 , 1089063872};	

	release_assert(ITER_COUNT == carray_len(seeds));
	println("remove_impossible:", REMOVE_IMPOSSIBLE);
	println("use arena_allocator:", ARENA_ALLOCATOR);

	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		println("iteration", iteration);
		data[0].add(run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT));
	}
	auto avrg = get_averages(data);
	print_averages(avrg);
}
void experiment_mcts_bootstrap() {
	DEPTH_LOWER_CUTOFF = 10;
	BOX_UPPER_CUTOFF = 9;
	release_assert(REMOVE_IMPOSSIBLE && ARENA_ALLOCATOR && DEPTH_LOWER_CUTOFF == 10 &&  BOX_UPPER_CUTOFF == 9 && BOX_LOWER_CUTOFF == 1 && USE_SIMPLE_MOVES == false);
	UCB1_C = 1.0/SQRT2;
	const i32 ITER_COUNT = 10;
	const f64 TIMEOUT = 30.0;	
	const i32 PARAM_COUNT = 4;
	f64 param[4] = {0.05, 0.1, 0.15, 0.20};
    
	auto data_first  = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	auto data_second = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, (i32)PARAM_COUNT) {
		data_first[i]  = make_array<Experiment_Info>(0, 100000);
		data_second[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT] = {1372869774 , 82247878 , 524804330 , 1081525251 , 1816911730 , 1958083045 , 574904445 , 1948822811 , 1211038774 , 231142504};
	release_assert(ITER_COUNT == carray_len(seeds));
	println("use bootstrap:", MCTS_BOOTSTRAP);
	#if MCTS_BOOTSTRAP
	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		
		println("iteration", iteration);
		println("seed:" , seed);		
		for_range(i, 0, (i32)PARAM_COUNT) {
			MCTS_BOOTSTRAP_DELTA = param[i];
			println("bt delta:", MCTS_BOOTSTRAP_DELTA);
			auto pair = run_experiment_mcts_timeout_bootstrap(seed, node_ucb1, TIMEOUT);
			data_first[i].add(pair.first);
			data_second[i].add(pair.second);
		}
	}
	#else 
	data_second.resize(1);
	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];		
		println("iteration", iteration);
		println("seed:" , seed);		
		auto pair = run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT);	
		data_second[0].add(pair);
	}
	#endif 
	auto avrg = get_averages(data_second);
	print_averages(avrg);	
}

// comparison between Mersenne vs Rand
// Mersenne even though it is slower (0.5 * Rand) seems to lead to *way* better results
// but since a proper comparison isn't possible for such a small amount of iterations it has not been further discussed
// in the paper.
void experiment_rng() {
	release_assert(!MCTS_BOOTSTRAP && REMOVE_IMPOSSIBLE && ARENA_ALLOCATOR && DEPTH_LOWER_CUTOFF == 7 &&  BOX_UPPER_CUTOFF == 9 && BOX_LOWER_CUTOFF == 1 && USE_SIMPLE_MOVES == false);
	const i32 ITER_COUNT = 10;
	const f64 TIMEOUT = 30.0;	
	const i32 PARAM_COUNT = 1;
	// MAX ^= infinity
	bool param[PARAM_COUNT] = {MT_RANDOM};
    
	auto data = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT] = {927255397 , 1105921952 , 1026354299 , 1758167147 , 1603941903 , 1676667435 , 532609334 , 182937063 , 1789453788 , 644406624};
	release_assert(ITER_COUNT == carray_len(seeds));
	println("use mersenne:", MT_RANDOM);

	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		println("iteration", iteration);
		println("seed", seed);
		data[0].add(run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT));
	}
	auto avrg = get_averages(data);
	print_averages(avrg);
}
// TREE_POLICY_NEXT vs RANDOM
// Not part of of the Result section
void experiment_expansion() {
	BOX_UPPER_CUTOFF = 9;
	release_assert(!MCTS_BOOTSTRAP && REMOVE_IMPOSSIBLE && ARENA_ALLOCATOR && DEPTH_LOWER_CUTOFF == 7 &&  BOX_UPPER_CUTOFF == 9 && BOX_LOWER_CUTOFF == 1 && USE_SIMPLE_MOVES == false);
	const i32 ITER_COUNT = 10;
	const f64 TIMEOUT = 30.0;	
	const i32 PARAM_COUNT = 1;
	// MAX ^= infinity
	bool param[PARAM_COUNT] = {TREE_POLICY_NEXT};
    
	auto data = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT] = {1449229093 , 1549061143 , 134623390 , 491201147 , 2136801405 , 2123956038 , 232390758 , 143488686 , 1284532748 , 1541494547};
	release_assert(ITER_COUNT == carray_len(seeds));
	println("use next:", TREE_POLICY_NEXT);

	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		println("iteration", iteration);
		println("seed", seed);
		data[0].add(run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT));
	}
	auto avrg = get_averages(data);
	print_averages(avrg);
}	

void run_with_tracking(u64 seed, f64 timeout = 30.0) {
	
	println("seed:", seed);
	Array<Score_Tracking_Data> data;
	run_experiment_mcts_timeout_bootstrap(seed, node_ucb1_tuned, timeout, &data);
	save_score_tracking_data(data);
	data.destroy();
	println("finished");
}
void run_with_tracking_no_bt(u64 seed, f64 timeout = 30.0) {
	
	println("seed:", seed);
	Array<Score_Tracking_Data> data;
	run_experiment_mcts_timeout(seed, node_ucb1_tuned, timeout, &data);
	save_score_tracking_data(data);
	data.destroy();
	println("finished");
}


// not presented in the paper
void experiment_action_delete_obstacle() {
	BOX_UPPER_CUTOFF = 9;
	release_assert(!MCTS_BOOTSTRAP && REMOVE_IMPOSSIBLE && ARENA_ALLOCATOR && DEPTH_LOWER_CUTOFF == 7 &&  BOX_UPPER_CUTOFF == 9 && BOX_LOWER_CUTOFF == 1 && USE_SIMPLE_MOVES == false);
	const i32 ITER_COUNT = 2;
	const f64 TIMEOUT = 60.0;	
	const i32 PARAM_COUNT = 1;
	// MAX ^= infinity
	bool param[PARAM_COUNT] = {true};
    
	auto data = make_array<Array<Experiment_Info>>(PARAM_COUNT);
	for_range(i, 0, data.count) {
		data[i] = make_array<Experiment_Info>(0, 100000);
	}
	
	u64 seeds[ITER_COUNT] = {62132460 , 17694};// , 498225673 , 1290082761 , 698030530};
	release_assert(ITER_COUNT == carray_len(seeds));
	// println("use next:", TREE_POLICY_NEXT);

	for_range(iteration, 0, (i32)ITER_COUNT) {
		u64 seed = seeds[iteration];
		println("iteration", iteration);
		println("seed", seed);
		data[0].add(run_experiment_mcts_timeout(seed, node_ucb1, TIMEOUT));
	}
	auto avrg = get_averages(data);
	print_averages(avrg);
}	

#endif // EXPERIMENT_H
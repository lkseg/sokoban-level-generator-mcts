#ifndef SETTINGS_H
#define SETTINGS_H
#include <stdint.h>

/*
    This file contains all general settings that are being used for random level generation.
    Some of these settings are being ignored during experiments since they are a bit
    more specialized.

    To see the direct usage of the mcts see the procedures:
    run_mcts_timeout and run_mcts_timeout_and_bootstrap
    in experiment.h.

    General entry point is in mcts.h/cpp : next_rollout/uct_body

    note: 
        MCTS_BOOTSTRAP is false by default.
        ADD_GOOD_LEVELS is a very important option with GOOD_LEVEL_CUT, PRINT_NEW_LEVEL_INFO
        For the scoring of the levels see 'score_node' in mcts.cpp.
*/


// {width, height}
// constraint: 16 <= x*y <= 254, so ~{16,15} is the max
// width != height is possible!
#define DEFAULT_BOARD_SIZE {7,7}

// Set to {-1,*} for middle (default).
// Must be either {-1,*} or inside DEFAULT_BOARD_SIZE
#define DEFAULT_START_POSITION {-1, 0} // {2, 3}

// Set to 0 if SIMULATION_COUNT should be used instead
#define DEFAULT_TIMEOUT 10.0

// If true will not only add new best levels but also levels whose score is >= GOOD_LEVEL_CUT.
// This also affects bootstrapping. Which all in all makes Bootstrapping too complicated to properly evaluate.
#define ADD_GOOD_LEVELS true
// 1.3 is generally a decent value thanks to the area scaling
#define GOOD_LEVEL_CUT 1.3
// Max amount of levels that will be usable at the end
#define LEVEL_SET_SIZE 30
// If true prints when a new good level is being added
#define PRINT_NEW_LEVEL_INFO true

// If it is set to 0 a random seed will be generated else it uses the seed
#define DEFAULT_SEED 0 
// Use the simple move action implementation (tile by tile) meant for experiments
#define USE_SIMPLE_MOVES false

// Enables bootstrapping
#define MCTS_BOOTSTRAP false
// How many levels should get added for bootstrapping
inline int32_t MCTS_BOOTSTRAP_COUNT = 4;
// How much of the timeout should be spend for bootstrapping
inline double MCTS_BOOTSTRAP_DELTA = 0.05;

// If set to true it uses the 5 example levels of the original paper and prints out their scores for comparison
// see sokoban_example_levels
#define USE_EXAMPLE_LEVELS false

// Enables usage of the arena allocator during default policy
#define ARENA_ALLOCATOR true


// Simulation count is being used if DEFAULT_TIMEOUT == 0
// Bootstrap does not work with this
#define SIMULATION_COUNT 800000



// Activates removal of impossible configurations
#define REMOVE_IMPOSSIBLE true


inline int32_t DEPTH_LOWER_CUTOFF = 10;


#define EXPERIMENT_INF 1000000

#define BOX_AREA_CUTOFF 3.0
// PLACE-BOX can not place more than that many boxes
// set to -1 to be area/BOX_AREA_CUTOFF
// EXPERIMENT_INF for max
inline int32_t BOX_UPPER_CUTOFF = -1;
// FREEZE-LEVEL has to have at least that many boxes
inline int32_t BOX_LOWER_CUTOFF = 1;

// true:  use mersenne twister
// false: use rand
#define MT_RANDOM true

// true:  expands next action type in tree policy
// false: expands totally random
#define TREE_POLICY_NEXT true

/*
    Actives the experiments.
    Some parts of the algorithm changes depending on this.
    Look at the beginning of the main function for an idea how this all works
    together with experiment.h.
    Some of the experiments always have certain requirement settings which can be 
    seen in their respective 'release_assert'.
    See 'if constexpr(EXPERIMENTS)' in main.cpp.
*/
#define EXPERIMENTS false

// in the mcts paper 2*C with C=1/sqrt(2), is being recommended as default
constexpr double SQRT2 = 1.41421356237;


inline double UCB1_C  = 2.0 * 1.0/SQRT2;
inline double SP_MCTS_D =  SQRT2;

// Can be defined by hand or through an scons argument
// This *heavily* slows down the program
#ifdef XTRACK
#define TRACK_DATA true
#else 
#define TRACK_DATA false
#endif // XTRACK

// For ucb1-tuned, ucb-v we have to save the squared sum score
// If we don't want to have the extra overhead for the other methods we can just remove this define
#define USE_SQUARED_SUM


inline int DEPTH_SOFT_CUTOFF = 3;
#endif // SETTINGS_H
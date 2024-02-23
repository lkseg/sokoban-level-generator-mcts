#ifndef MCTS_H
#define MCTS_H

#include "util.h"
#include "sokoban.h"
#include "settings.h"
                                    //  up      right   down    left
const Vector2i DIRECTION_TO_VEC[4] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
inline u8 get_direction(const Vector2i &v) {
    if(v.x == 0) {
        if(v.y==-1) return 0;
        return 2;
    } else if(v.x == 1) {
        return 1;
    }
    return 3;
}

enum Mcts_Node_Flag : u8 {
    MCTS_BLOOMED        = 1 << 0,
    MCTS_SECOND_ACTION  = 1 << 1,  // second action_set after 'freezing'
    MCTS_TERMINAL       = 1 << 2,
    MCTS_EXPANDED       = 1 << 3,
    MCTS_CAN_FREEZE     = 1 << 4,
    MCTS_EVALUATED      = 1 << 5,
    MCTS_FROZEN         = 1 << 6,
};

#define is_bloomed(NODE)  bool(NODE->flags & MCTS_BLOOMED)
#define is_terminal(NODE) bool(NODE->flags & MCTS_TERMINAL)


#define INVALID_INDEX u8(255)

struct Mcts_Node;
struct Mcts;
// ucb1, ucb1-tuned etc.
typedef f64(*Decision_Proc)(Mcts_Node *);

void uct_body(Mcts *, const Decision_Proc);

Mcts_Node *tree_policy(Mcts_Node *, Mcts *, const Decision_Proc);
f64 default_policy(Mcts_Node *, Mcts *);


void bloom(Mcts_Node *, Mcts *);
Mcts_Node *expand_random(Mcts_Node *, Mcts *);
Mcts_Node *expand_next(Mcts_Node *, Mcts *);

Mcts_Node *best_child(Mcts_Node *, Mcts *, const Decision_Proc);

f64 get_score_scale(Mcts *);
Mcts *new_mcts(u64, Vector2i = {5,5}, Vector2i = {-1, -1});
Mcts *new_mcts_bootstrap(u64, Vector2i, Vector2i);
void root_add_custom_child(Mcts *, Grid &, f64);

void delete_mcts(Mcts *);
Mcts_Node make_mcts_node(Mcts_Node &);
Mcts_Node clone_mcts_node(Mcts_Node &, Mcts_Node *);
isize get_box_count(Grid &);

struct Move_Info {
    u8 index;
    u8 direction;
};

// See next_rollout/uct_body for the entry point of the algorithm
struct Mcts {
    Mcts_Node *root;
    u64 seed;
    f64 best_score = -1;

    Array<Level> finished_nodes;
    
    Vector2i size;
    i32 start_position;
    Vector2i start_position_tile;
    f64 area;
    f64 score_scale;    
    u32 last_rollout_depth = 0;
    i32 box_upper_cutoff;
    i32 depth_soft_cutoff; // Soft cutoff which is not being used
    Chrono_Clock time_start;
    // bool no_delete = false;
    // only used in bootstrapping
    bool finish_early = false;    
    force_inline void next_rollout(const Decision_Proc decision) {
        uct_body(this, decision);
    }
    // used for experiments returns info
    f64 experiment_rollout(Mcts *, const Decision_Proc);

    Array<Level> get_level_set(isize count = 20);   
    void start();
};

struct Mcts_Node {
    
    f64 score_sum = 0;

    #ifdef USE_SQUARED_SUM
    f64 squared_score_sum = 0;
    #endif // USE_SQUARED_SUM

    Mcts_Node *parent;
    
    Array<Mcts_Node *> children;

    // See actions for the usage
    // This has been discussed in the paper    
    Array<u8> first; 
    Array<u8> second; 
    Array<Move_Info> moves;
    // Every node has their own grid
    Grid grid;

    i32 rollout_count = 0;
    i16 box_count = 0;
    u16 depth = 0;
    u8 flags = 0;
    u8 pusher;    
    
    
    

    inline bool can_freeze() {
        return !(flags&MCTS_FROZEN) && (flags&MCTS_CAN_FREEZE) && (depth>=DEPTH_LOWER_CUTOFF);
    }
    inline bool can_expand() {
        assert( !(flags & MCTS_TERMINAL) && (flags & MCTS_BLOOMED));
        
        if(flags & MCTS_SECOND_ACTION) {
            return (moves.count>0) || !(flags & MCTS_EVALUATED);
        }
        bool b1 = first.count > 0;
        bool b2 = second.count > 0;
        bool b3 = can_freeze();
        return b1 || b2 || b3;
    }
    void add_score_and_propagate(f64 score);

    void destroy();
};

void rollout(Mcts_Node*);

void remove_impossible_v1(Mcts_Node *);
void remove_impossible_v2(Mcts_Node *);


f64 score_node(Mcts_Node &, Mcts *);
f64 score_node_test(Mcts_Node &, Mcts *, f64, f64, f64, f64, f64, f64, f64);

template<typename T>
inline void node_data_remove(Array<T> &data, isize index) {
    swap_values(data[index], data[data.count-1]);
    data.count -= 1;
    if(data.count <= 0) {
        data.destroy();
    }
}

/* template<typename T>
inline void node_data_remove(Static_Array<T, 4> &data, isize index) {
    swap_values(data[index], data[data.count-1]);
    data.count -= 1;
} */

inline Mcts_Node *new_node_child(Mcts_Node &parent, Mcts *tree) {
    Mcts_Node *child = mem_alloc<Mcts_Node>();
    *child = make_mcts_node(parent);
    parent.children.add(child);
    return child;
}

// prints
void print_move_infos(Array<Move_Info> &, Grid &);
void print_node_debug(Mcts_Node &);
void print_node(Mcts_Node &node);
void print_children(Mcts_Node *node);
std::ostream &operator<<(std::ostream &, const Mcts &);
void print_scored_nodes(Mcts &);



void debug_check_box_count(Mcts_Node &node, const char *msg = nullptr);
#include "uct_enhancements.h"

#endif // MCTS_H




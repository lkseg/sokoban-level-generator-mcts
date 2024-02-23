#ifndef MCTS_ACTIONS_H
#define MCTS_ACTIONS_H

#include "mcts.h"
/*
    Following we have the implementations of the 5 actions.
    The have been implemented as follows:
        First action_* which produces the necessary information for applying the
        action * to the node for the expansion.

        The action is then being applied for the expansion with
        new_* which returns a new child node with action * being applied.

*/

/*
    Here we have actually 2 implementations.
    As discussed in the paper this one performs generally better.
*/
inline void action_delete_obstacle(Mcts_Node &node, Mcts *tree) {
    /* if(tree->no_delete) {
        return;
    } */
    assert((node.first.count == 0));
    Vector2i tile;
    for_grid(tile, node.grid) {            
        if(!pawn_is_block(node.grid.get(tile))) continue;
        for_range(direction, 0, 4) {
            Vector2i v = tile + DIRECTION_TO_VEC[direction];
            if(node.grid.in_grid(v.x, v.y) && !pawn_is_block(node.grid.get(v))) {
                node.first.add(node.grid.as_index(tile));
                break;
            }
        }        
    }    
}

inline void alternative_action_delete_obstacle(Mcts_Node &node, Mcts *tree) {
    /* if(tree->no_delete) {
        return;
    } */
    assert((node.first.count == 0));
    // two empty tiles can 'mark' the same block so we have to save
    // all the used blocks
    auto is_used = make_array<bool>(node.grid.get_count());
    init_data(is_used, false);

    Vector2i tile;
    for_grid(tile, node.grid) {            
            if(pawn_is_block(node.grid.get(tile))) continue;
            for_range(direction, 0, 4) {
                Vector2i v = tile + DIRECTION_TO_VEC[direction];
                i32 idx = node.grid.as_index(v.x, v.y);
                if(node.grid.in_grid(v.x, v.y) && !is_used[idx] && pawn_is_block(node.grid.get(idx))) {
                    assert(idx<=255);
                    node.first.add(idx);
                    is_used[idx] = true;
                }
            }
    }
    is_used.destroy();
    
}

inline Mcts_Node *new_delete_obstacle(Mcts_Node &node, Mcts *tree) {

    assert(node.first.count > 0);

    auto child = new_node_child(node, tree);
    
    auto rand_idx = randi_range(0, node.first.count-1);
    auto idx = node.first[rand_idx];
    assert(node.grid.get(idx) == Pawn::Block);
    child->grid.set(idx, Pawn::Empty);

    // 'hide' the value such that it can't be picked again
    node_data_remove(node.first, rand_idx);

    return child;
}
// place a block into an empty tile
inline void action_place_box(Mcts_Node &node, Mcts *tree) {    
    if(node.box_count >= tree->box_upper_cutoff) return;//|| node.box_count >= node.depth/2) return;
    assert((node.second.count == 0));

    for_range(i, 0, node.grid.get_count()) {
        Pawn it = node.grid.data[i];
        if(pawn_is_empty(it) && i != tree->start_position) {
            assert(i<=255);
            node.second.add(i);
        }
    }
} 

inline Mcts_Node *new_place_box(Mcts_Node &node, Mcts *tree) {

    assert(node.second.count > 0);
        
    auto child = new_node_child(node, tree);
    
    auto rand_idx = randi_range(0, node.second.count-1);
    auto idx = node.second[rand_idx];
    assert(node.grid.get(idx) == Pawn::Empty);
    child->grid.set(idx, Pawn::Box);
    child->box_count += 1;
    node_data_remove(node.second, rand_idx);

    return child;
}
inline void action_freeze(Mcts_Node &node) {
    if(node.box_count >= BOX_LOWER_CUTOFF) {
        node.flags |= MCTS_CAN_FREEZE;
    }
}
inline Mcts_Node *new_freeze_level(Mcts_Node &node, Mcts *tree) {
    

    node.flags |= MCTS_FROZEN;        

    auto child = new_node_child(node, tree);
    child->flags |= MCTS_SECOND_ACTION;
    
    if(pawn_is_box(child->grid.get(tree->start_position))) {
        // we override it
        child->box_count -= 1;
        child->grid.set(tree->start_position, Pawn::Empty);
    }
    if_debug {
        if(child->pusher != tree->start_position) {
            println(child->pusher, tree->start_position);
            assert(false);
        }
        assert(pawn_is_empty(child->grid.get(tree->start_position)));
    }

    // saves the start position of the boxes
    child->first.resize(child->grid.get_count()); 
    // saves the move count of the boxes
    child->second.resize(child->grid.get_count()); 

    // ----------------

    if constexpr (REMOVE_IMPOSSIBLE) {
        // v1 doesn't perform well, while v2 does
        remove_impossible_v2(child);
    }
    
    // ---------------
    for_range(i, 0, child->grid.get_count()) {
        if(pawn_is_box(child->grid.get(i))) {
            child->first[i] = i; // box start position
            child->second[i] = 0; // move counter
        } else {
            child->first[i]  = INVALID_INDEX;
            child->second[i] = INVALID_INDEX;
        }
    }
    if_debug {
        debug_check_box_count(node, "freeze parent");        
        debug_check_box_count(*child, "freeze child");        
    }
    return child;
}

void _get_all_possible_moves(Vector2i tile, Grid &grid, Array<bool> &visited, Array<Move_Info> &);
void _get_all_possible_moves_iterative(Vector2i tile, Grid &grid, Array<bool> &visited, Array<Move_Info> &);

inline Array<Move_Info> get_all_possible_moves(Vector2i pusher, Grid &grid) {
    Array<bool> visited = make_array<bool>(grid.get_count());

    init_data(visited, false);
    visited[grid.as_index(pusher.x, pusher.y)] = true;
    auto moves = make_array<Move_Info>(0, 5);
    _get_all_possible_moves(pusher, grid, visited, moves);
    visited.destroy();
    return moves;
}

inline Array<Move_Info> generate_all_possible_moves(Mcts_Node &node) {
    if(node.box_count == 0) {
        return {};
    }
    assert(pawn_is_empty(node.grid.get(node.pusher)));
    auto moves = get_all_possible_moves(node.grid.as_tile(node.pusher), node.grid);
    if constexpr (false) {
        println(str(node.grid));
        for_range(i, 0, moves.count) {
            println(node.grid.as_tile(moves[i].index), "  ", DIRECTION_TO_VEC[moves[i].direction]);
        }
    }
    return moves;
}

// this is the simple variant of the function which only moves one tile
// at the time
// used for comparison in the experiments
inline Array<Move_Info> simple_move_agent(Mcts_Node &node, Mcts *tree) {
    if(node.box_count == 0) {
        return {};
    }
    auto pos = node.grid.as_tile(node.pusher);
    u8 pos_i = node.pusher;
    auto moves = make_array<Move_Info>(0, 5);
    for_range(direction, 0, 4) {
        Vector2i v = DIRECTION_TO_VEC[direction];        
        if(node.grid._could_move(pos.x, pos.y, v)) {
            moves.add(Move_Info{pos_i, (u8)direction});
        }
    }
    return moves;
}

inline void action_move_agent(Mcts_Node &node, Mcts *tree) {
    assert(node.moves.count == 0);
    assert((node.flags & MCTS_SECOND_ACTION));
    if constexpr (!USE_SIMPLE_MOVES) {
        node.moves = generate_all_possible_moves(node);
    } else {
        node.moves = simple_move_agent(node, tree);
    }
}




inline Mcts_Node *new_move_agent(Mcts_Node &_node, Mcts *tree) {
    if_debug {
        debug_check_box_count(_node, "move start");
        assert(pawn_is_empty(_node.grid.get(_node.pusher)));
    }


    assert(_node.moves.count>0);
    
    
    auto child = new_node_child(_node, tree);

    auto rand_idx = randi_range(0, _node.moves.count-1);
    auto pusher_idx = _node.moves[rand_idx].index;
    assert(_node.moves[rand_idx].direction<4);
    Vector2i d = DIRECTION_TO_VEC[_node.moves[rand_idx].direction];
    //child->pusher = pusher_idx;
    
    
    Vector2i pusher = child->grid.as_tile(pusher_idx);
    auto move_to = child->grid.as_index(pusher + d);


    bool is_box = pawn_is_box(child->grid.get(move_to));
    assert(pusher.x>=0);
    
    // println("pre:\n", grid_as_string(child->grid));
    if(is_box) {
        assert(pawn_is_box(child->grid.get(move_to)));
        assert( (d.x != d.y) && ( (abs(d.x) == 1) || abs(d.y) == 1) );
        // new pusher positon:  pusher + d = tile
        // new box position:    pusher + 2*d = tile + d
        Vector2i v = pusher + 2*d;        
        assert(child->grid.in_grid(v.x, v.y) && !pawn_has_collision(child->grid.get(v)));
        auto push_pos = child->grid.as_index(v.x, v.y);


        child->grid.swap_top_layer(push_pos, move_to);
        child->grid.swap_top_layer(move_to, pusher_idx);
        assert(pawn_is_box(child->grid.get(push_pos)));
        if_debug {
            if(pawn_has_collision(child->grid.get(move_to))) {
                println_str(_node.grid);
                println_str(child->grid);
                println(pusher, d);
                print_move_infos(_node.moves, _node.grid);
                assert(false);
            }           
        }

        assert(child->first[push_pos] == INVALID_INDEX);
        assert(child->first[move_to]  != INVALID_INDEX);
        assert(child->second[push_pos] == INVALID_INDEX);
        assert(child->second[move_to]  != INVALID_INDEX);
         
        child->first[push_pos] = child->first[move_to];
        // increase move_count for the specific box
        child->second[push_pos] = child->second[move_to] + 1;

        child->first[move_to] =  INVALID_INDEX;
        child->second[move_to] = INVALID_INDEX;
        
    } else {
        // we can just move
        child->grid.swap_top_layer(pusher_idx, move_to);
        // this part should get never hit with optimized move_agent
        assert(USE_SIMPLE_MOVES);
    }
    child->pusher = move_to;
    node_data_remove(_node.moves, rand_idx);
    if_debug {
        debug_check_box_count(*child, "move end");
    }
    return child;
}

inline Mcts_Node *new_evaluate_level(Mcts_Node &_node, Mcts *tree) {

    _node.flags |= MCTS_EVALUATED; 
    auto child = new_node_child(_node, tree);

    child->flags |= MCTS_TERMINAL;

    for_range(i, 0, child->grid.get_count()) {
        if(pawn_is_box(child->grid.data[i])) {
            i32 move_count = child->second[i];
            assert(move_count>=0);

            // replace with Block
            if(move_count == 0) {
                assert(child->first[i] == i);
                child->grid.data[i] = Pawn::Block;
                child->box_count -= 1;

                child->first[i] = INVALID_INDEX;
                child->second[i] = INVALID_INDEX;
                continue;
            }
            
            // replace with Empty
            if(move_count == 1) {
                // DOS(A) bool(!grid.in_grid(A.x, A.y) || pawn_is_block(grid.get(A.x, A.y)))
                child->grid.data[i] = Pawn::Empty;
                child->box_count -= 1;

                child->first[i] = INVALID_INDEX;
                child->second[i] = INVALID_INDEX;                

                continue;
            }
            continue;
            // other experiments with the post processing            
            auto from = child->grid.as_tile(i);
            auto to = child->grid.as_tile(child->first[i]);
            auto d = to - from;
            d = {abs(d.x), abs(d.y)};
            auto m = d.x + d.y;
            if(m <= 1) {
                child->grid.data[i] = Pawn::Empty;
                child->box_count -= 1;

                child->first[i] = INVALID_INDEX;
                child->second[i] = INVALID_INDEX;                
                continue;
            }
        }
    }
    // replace current box positions with goals
    for_range(i, 0, child->grid.get_count()) {
            if(pawn_is_box(child->grid.data[i])) {
                // assert(child->second[i] >= 0);
                child->grid.data[i] = Pawn::Goal;

                // this is the start position of the goal                
                auto start = child->first[i];
                // we save in second, at the start position, the goal position
                child->second[start] = i;
            }
    }

    if_debug {
        for_range(i, 0, child->grid.get_count()) {
            assert(!pawn_is_box(child->grid.data[i]));
        }
    }


    // place the boxes at their start position
    for_range(i, 0, child->grid.get_count()) {
        auto start = child->first[i];
        if(start != INVALID_INDEX) {
            u8 bot = bot_layer(child->grid.data[start]);
            child->grid.set(start, Pawn(u8(Pawn::Box) | bot));
            
            assert(pawn_is_box(child->grid.data[start])); 
        }
    }

    // set pusher to start position
    u8 bot = (u8)bot_layer(child->grid.get(tree->start_position));
    child->grid.set(tree->start_position, Pawn(u8(Pawn::Pusher) | bot));

    if_debug {
        isize g_count = 0;
        isize b_count = 0;
        for_range(i, 0, child->grid.get_count()) {
            auto pawn = child->grid.data[i];
            if(pawn_is_goal(pawn)) g_count += 1;
            if(pawn_is_box(pawn)) b_count += 1;
        }
        if (b_count != child->box_count) {
            println(str(child->grid));
            println(b_count, child->box_count);
            assert(false);
        }
        assert(g_count == b_count && g_count == child->box_count);
    }
    return child;
}








#endif // MCTS_ACTIONS_H
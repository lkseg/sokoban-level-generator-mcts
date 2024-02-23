#include "mcts.h"
#include "mcts_actions.h"
#include "allocator.h"
#include "settings.h"

Mcts_Node *bloom_and_check_expand(Mcts_Node *, Mcts *);

void uct_body(Mcts *tree, const Decision_Proc decision) {
    auto node = tree_policy(tree->root, tree, decision);
    f64 score = default_policy(node, tree);
    node->add_score_and_propagate(score);    
}
f64 experiment_rollout(Mcts *tree, const Decision_Proc decision) {
    auto node = tree_policy(tree->root, tree, decision);
    f64 score = default_policy(node, tree);
    node->add_score_and_propagate(score);    
    return score;
}

Mcts_Node *tree_policy(Mcts_Node *node, Mcts *tree, const Decision_Proc decision) {

    while( !(node->flags & MCTS_TERMINAL)) {
        if(!is_bloomed(node)) {
            node = bloom_and_check_expand(node, tree);                
            assert(node);
            
        } else {
            if(node->can_expand()) {
                #if TREE_POLICY_NEXT == true
                    return expand_next(node, tree);
                #else 
                    return expand_random(node, tree);
                #endif
            }
            node = best_child(node, tree, decision);
        }
    }
    
    return node;    
}
f64 default_policy(Mcts_Node *base, Mcts *tree) {    
    tree->last_rollout_depth = base->depth;
    if(base->flags & MCTS_TERMINAL) {
        return score_node(*base, tree);
    }
    assert(base->children.count == 0);
    
    
    #if ARENA_ALLOCATOR
    global_arena_allocator->clear_arena();
    global_allocator = global_arena_allocator;
    #endif // ARENA_ALLOCATOR

    Mcts_Node _node;
    // simulation on a cloned node
    clone_mcts_node(_node, base);
    _node.parent = nullptr;
    //assert(_node.moves.data != base->moves.data);
    assert(_node.moves.count == base->moves.count);
    if_debug {
        for_range(i, 0, _node.moves.count) {
            assert(_node.moves[i].index == base->moves[i].index);
        }
    }
    assert(_node.children.data == nullptr);
    assert(_node.children.count == base->children.count);

    Mcts_Node *node = &_node;

    // Mcts_Node *_debug_base_copy = &_node;

    while(! (node->flags & MCTS_TERMINAL)) {
        if(!is_bloomed(node)) {
            node = bloom_and_check_expand(node, tree);
            assert(node);
            if(!node) break;
        } else {
            assert(node->can_expand());
            node = expand_random(node, tree);
        }
    }

    #if ARENA_ALLOCATOR
    global_allocator = global_default_allocator;
    #endif // ARENA_ALLOCATOR

    #if MCTS_BOOTSTRAP
    if(!node) {
        #if ARENA_ALLOCATOR == false
        _node.destroy();
        #endif // ARENA_ALLOCATOR
        return 0;
    }
    #endif // MCTS_BOOTSTRAP

    f64 score = score_node(*node, tree);    
    if(score > tree->best_score) {
        #if EXPERIMENTS
        tree->finished_nodes.add(make_level(node->grid, node->box_count, score, get_time()));
        #else
        println("new best (score | time):", score, time_diff(tree->time_start, get_time()));
        tree->finished_nodes.add(make_level(node->grid, node->box_count, score));
        #endif 
        tree->best_score = score;
    } else if( ADD_GOOD_LEVELS && score >= GOOD_LEVEL_CUT) {

        #if EXPERIMENTS
        tree->finished_nodes.add(make_level(node->grid, node->box_count, score, get_time()));
        #else
        if constexpr(PRINT_NEW_LEVEL_INFO) {
            println("new good level:", score);
        }
        tree->finished_nodes.add(make_level(node->grid, node->box_count, score));
        #endif 
    }

    #if ARENA_ALLOCATOR == false
    _node.destroy();
    #endif // ARENA_ALLOCATOR

    return score;
}
void Mcts_Node::add_score_and_propagate(f64 score) {
    rollout_count += 1;
    score_sum += score;

    #ifdef USE_SQUARED_SUM
    auto squared = score*score;
    squared_score_sum += squared;
    #endif // USE_SQUARED_SUM
    
    for(auto node = parent; node; node = node->parent) {
        node->score_sum += score;
        node->rollout_count += 1;

        #ifdef USE_SQUARED_SUM
        node->squared_score_sum += squared;
        #endif // USE_SQUARED_SUM
    }
}

void prune_node(Mcts_Node *node) {
    auto parent = node->parent;
    assert(parent);
    parent->children.remove_match(node);
    node->destroy();
    mem_free(node);
}

Mcts_Node *bloom_and_check_expand(Mcts_Node *node, Mcts *tree) {
    assert(!is_bloomed(node) && !is_terminal(node));

    auto parent = node->parent;
    bloom(node, tree);
    // This pretty much never gets triggered.
    // This part is also one of the reasons why having an even more invasive
    // depth limit beyond freeze is too problematic.
    // During the tree_policy one can simply 'move back' if no bloom/expansion is possible and prune
    // the branch.
    // But during the default_policy such a thing isn't trivial. 
    // Therefore evaluate_level has no requirements.
    if(!node->can_expand()) {
        // println("can't expand");
        // assert(false);
        if(node->parent == nullptr) {
            // println(str(node->grid), node->flags & MCTS_FROZEN);
            return nullptr;
        }
        prune_node(node);
        assert(parent);
        // This part gets triggered very rarely.
        while(parent->children.count == 0 && !parent->can_expand()) {
            node = parent;
            parent = parent->parent;
            // This part should generally only get triggered when bootstrapping.
            if(!parent || (!(parent->flags&MCTS_TERMINAL))) {
               //  println(str(node->grid), node->flags & MCTS_FROZEN);
                return nullptr;
            }
            prune_node(node);            
        }
        node = parent;
    }    
    return node;

}



Mcts_Node *best_child(Mcts_Node *node, Mcts *tree, const Decision_Proc decision) {
    f64 max_val = F64_MIN;
    isize arg = -1;
    assert(node->children.count > 0);
    for_range(i, 0, node->children.count) {
        Mcts_Node *it = node->children[i];
        f64 val = decision(it);
        if(val > max_val) {                
            max_val = val;
            arg = i;
        }        
    }
    if_debug {
        if(arg<0) {
            println(str(node->grid));
            for_range(i, 0, node->children.count) {
                Mcts_Node *it = node->children[i];
                println(it->score_sum, it->squared_score_sum, it->parent->rollout_count, it->rollout_count);
                f64 val = decision(it);
                println(val);
            }            
        }
    }
    assert(arg >= 0);
    return node->children[arg]; 
}
void bloom(Mcts_Node *node, Mcts *tree) {
    assert(!(node->flags & MCTS_EXPANDED) && !(node->flags & MCTS_BLOOMED) && !(node->flags & MCTS_TERMINAL));
    if(node->flags & MCTS_SECOND_ACTION) {
        action_move_agent(*node, tree);
        // No action_evaluate since there is no special requirement to it besides being frozen.
        // For a comment on that see *bloom_and_check_expand*.
    } else {
        action_delete_obstacle(*node, tree);
        action_place_box(*node, tree);
        action_freeze(*node);
    }
    node->flags |= MCTS_BLOOMED;
}

Mcts_Node *expand_random(Mcts_Node *node, Mcts *tree) {
    assert(!(node->flags & MCTS_TERMINAL) && (node->flags&MCTS_BLOOMED));
    assert(node->can_expand());
    i64 A_COUNT = node->children.count;
    // Getting randomly the next child is a bit complicated since one
    // has to combine the expansion information.
    if(node->flags & MCTS_SECOND_ACTION) {
        bool e = !(node->flags&MCTS_EVALUATED);
        bool m = (node->moves.count > 0);
        

        assert(m | e);
        /* if(m > 0 && node->depth <= tree->depth_soft_cutoff) {
            new_move_agent(*node, tree);
        } else */
        if(m && e) {
            auto r = randi_range(0, node->moves.count);
            if(r == 0) {
                new_evaluate_level(*node, tree);
            } else {
                new_move_agent(*node, tree);
                
            }
        } else if(m) {
            new_move_agent(*node, tree);
        } else {
            new_evaluate_level(*node, tree);
        }
        
    } else {
        bool f = node->can_freeze();
        bool d = (node->first.count > 0);
        bool p = (node->second.count > 0);

        assert(d | p | f);

        auto r = randi_range(0, node->first.count+node->second.count);
        if(f && (r == 0)) {
            new_freeze_level(*node, tree);
        } else if(d && p) {
            if(r <= node->first.count) {
                new_delete_obstacle(*node, tree);
            } else {
                new_place_box(*node, tree); 
            }
        } else if(d) {
            new_delete_obstacle(*node, tree);
        } else {
            assert(p);
            new_place_box(*node, tree);
        } 
    }
    assert(A_COUNT+1 == node->children.count);
    node->flags |= MCTS_EXPANDED;
    return node->children[node->children.count-1];
}

// just create the next child
Mcts_Node *expand_next(Mcts_Node *node, Mcts *tree) {
    assert(!(node->flags & MCTS_TERMINAL) && (node->flags&MCTS_BLOOMED));
    assert(node->can_expand());

    node->flags |= MCTS_EXPANDED;
    if(node->flags & MCTS_SECOND_ACTION) {  
        bool m = (node->moves.count > 0);
        bool e = !(node->flags&MCTS_EVALUATED);
        assert(m | e);
        /* if( m > 0 && node->depth <= tree->depth_soft_cutoff) {
            return new_move_agent(*node, tree);
        } */
        if(m) {
            return new_move_agent(*node, tree);
        } else {
            return new_evaluate_level(*node, tree);
        }
    } else {
        bool d = (node->first.count > 0);
        bool p = (node->second.count > 0);
        bool f = node->can_freeze();
        assert(d | p | f);
        if(d) {
            return new_delete_obstacle(*node, tree);
        } else if(p) {
            return new_place_box(*node, tree);
        } else {
            return new_freeze_level(*node, tree);
        }
    }
}



Mcts_Node make_mcts_node(Mcts_Node &parent) {

    Mcts_Node node = {};
    node.parent = &parent;
    node.grid = clone_grid(parent.grid);
    node.box_count = parent.box_count;
    // println("box_count: ", parent.second.count, " : ", parent.box_count);
    node.depth = parent.depth + 1;
    node.pusher = parent.pusher;
    if(parent.flags & MCTS_SECOND_ACTION) {
        node.flags = MCTS_SECOND_ACTION;
        node.first = clone_array(parent.first);
        node.second = clone_array(parent.second);
    } else {
        node.flags = 0;
    }
    return node;
}
Mcts_Node clone_mcts_node(Mcts_Node &c, Mcts_Node *node) {
    c.parent = node->parent;
    c.children = clone_array(node->children);

    c.first = clone_array(node->first);
    c.second = clone_array(node->second);
    c.moves = clone_array(node->moves);

    c.pusher = node->pusher;
    

    c.grid = clone_grid(node->grid);
    c.flags = node->flags;
    c.score_sum = node->score_sum;
    #ifdef USE_SQUARED_SUM
    c.squared_score_sum  = node->squared_score_sum;
    #endif
    c.rollout_count = node->rollout_count;
    c.box_count = node->box_count;
    c.depth = node->depth;
    return c;
}
void Mcts_Node::destroy() {
    for_range(i, 0, children.count) {
        Mcts_Node *it = children[i];
        it->destroy();
        mem_free(it);
    }
    children.destroy();
    first.destroy();
    second.destroy();
    moves.destroy();
    grid.destroy();
}
// The terrain method from the first paper
i32 terrain_of(Grid &grid) {
    i32 good_tile_count = 0;
    for(i32 y = 0; y < grid.height; y+=1) {
            for(i32 x = 0; x < grid.width; x+=1) {
            Pawn pawn = grid.get(x, y);
            if(! (pawn_is_box(pawn))) {
                continue;
            }
            const Vector2i u = {x, y-1};
            const Vector2i r = {x+1, y};
            const Vector2i d = {x, y+1};
            const Vector2i l = {x-1, y};

            const Vector2i ur = {x+1, y-1};
            const Vector2i dr = {x+1, y+1};
            const Vector2i dl = {x-1, y+1};
            const Vector2i ul = {x-1, y-1};

            #define DOS(A) if(grid.in_grid(A.x, A.y) && pawn_is_block(grid.get(A.x, A.y))) good_tile_count += 1; 
            DOS(u);
            DOS(r);
            DOS(d);
            DOS(l);

            DOS(ur);
            DOS(dr);
            DOS(dl);
            DOS(ul);
            #undef DOS
            continue;
            // 3x3 area
            /* for(i32 y_i = -1; y_i <= 1; y_i += 1) {
                i32 iy = y+y_i;
                // shouldnt break since 0-1 is bad but 0+0 is ok
                if(iy >= grid.height || iy<0) continue;
                for(i32 x_i = -1; x_i <= 1; x_i += 1) {
                    i32 ix = x+x_i;
                    // if(x_i == 0 && y_i == 0) continue;
                    if(ix >= grid.width || ix<0) continue;
                    Pawn other = grid.get(ix, iy);
                    if(pawn_is_block(other)) {
                        good_tile_count += 1;
                    }
                }
            } */
        }
    }
    return good_tile_count;
}

// Different implementations for the terrain replacement of the second paper,
// since the paper wasn't perfectly clear on its counting method.
i32 area_score_of(Grid &grid) {
    i32 good_tile_count = 0;
    for_range(y, 1, grid.height-1) {
        for_range(x, 1, grid.width-1) {
            bool has_empty = false;
            bool has_block = false;

            // 3x3 area
            for(i32 y_i = -1; y_i <= 1; y_i += 1) {
                i32 iy = y+y_i;
                for(i32 x_i = -1; x_i <= 1; x_i += 1) {
                    if(x_i == 0 && y_i == 0) {
                        continue;
                    }
                    i32 ix = x+x_i;
                    Pawn pawn = grid.get(ix, iy);
                    has_empty = has_empty | (!pawn_is_goal(pawn) && !pawn_is_block(pawn));
                    has_block = has_block | pawn_is_block(pawn);
                    if(has_empty && has_block) {
                        good_tile_count += 1;
                        goto exit_area_loop;
                    }
                }
            }
            exit_area_loop:
            ((void)0);
        }
    }
    return good_tile_count;
}

i32 area_score_of_v2(Grid &grid) {
    i32 good_tile_count = 0;
    for_range(y, 1, grid.height-1) {
        for_range(x, 1, grid.width-1) {
            bool has_empty = false;
            bool has_block = false;

            // 3x3 area
            for(i32 y_i = -1; y_i <= 1; y_i += 1) {
                i32 iy = y+y_i;
                for(i32 x_i = -1; x_i <= 1; x_i += 1) {
                    i32 ix = x+x_i;
                    Pawn pawn = grid.get(ix, iy);
                    // has_empty = has_empty | (!pawn_is_goal(pawn) && !pawn_is_block(pawn));
                    has_empty = has_empty || !pawn_is_block(pawn);
                    has_block = has_block || pawn_is_block(pawn);
                    if(has_empty && has_block) {
                        good_tile_count += 1;
                        goto exit_area_loop;
                    }
                }
            }
            exit_area_loop:
            ((void)0);
        }
    }
    return good_tile_count;
}
// This one moves in a 3x3 block around the upper left cell.
// While area_score_of_v2 moves around the middle cell of each block.
// Both methods are equivalent.
i32 area_score_of_v3(Grid &grid) {
    i32 good_tile_count = 0;
    for_range(y, 0, grid.height-2) {
        for_range(x, 0, grid.width-2) {
            bool has_empty = false;
            bool has_block = false;

            // 3x3 area
            for_range(y_i, 0, 3){
                i32 iy = y+y_i;
                for_range(x_i, 0, 3) {
                    i32 ix = x+x_i;
                    Pawn pawn = grid.get(ix, iy);
                    // has_empty = has_empty | (!pawn_is_goal(pawn) && !pawn_is_block(pawn));
                    has_empty = has_empty || !pawn_is_block(pawn);
                    has_block = has_block || pawn_is_block(pawn);
                    if(has_empty && has_block) {
                        good_tile_count += 1;
                        goto exit_area_loop;
                    }
                }
            }
            exit_area_loop:
            ((void)0);
        }
    }
    return good_tile_count;
}
i32 area_score_of_v4(Grid &grid) {
    i32 good_tile_count = 0;
    const i32 limx = grid.width-1;
    const i32 limy = grid.height-1;
    for_range(y, 0, grid.height) {
        for_range(x, 0, grid.width) {
            bool has_empty = false;
            bool has_block = false;

            // 3x3 area
            for_range(y_i, 0, 3) {
                i32 iy = y+y_i;
                for_range(x_i, 0, 3) {
                    i32 ix = x+x_i;
                    if(ix>limy || iy>limx) continue;
                    Pawn pawn = grid.get(ix, iy);
                    // has_empty = has_empty | (!pawn_is_goal(pawn) && !pawn_is_block(pawn));
                    has_empty = has_empty || !pawn_is_block(pawn);
                    has_block = has_block || pawn_is_block(pawn);
                    if(has_empty && has_block) {
                        good_tile_count += 1;
                        goto exit_area_loop;
                    }
                }
            }
            exit_area_loop:
            ((void)0);
        }
    }
    return good_tile_count;
}


i32 x_symmetry_value(Mcts_Node &node) {
    Vector2i tile;
    i32 mid = node.grid.width/2;
    i32 stride = node.grid.width - 1;
    i32 count = 0;
    for_grid(tile, node.grid) {
        auto it = node.grid.get(tile);
        if(tile.x >= mid) continue;
        
        auto other = node.grid.get(stride-tile.x, tile.y);
        if(other == it) {
            if(pawn_is_box(it)) {
                count += 3;
            } else {
                count += 1;
            }
        }
    }
    return count;
}

i32 diagonal_symmetry_value(Mcts_Node &node) {
    Vector2i tile;
    i32 mid = node.grid.width/2;
    i32 stride = node.grid.width - 1;
    i32 count = 0;
    for_grid(tile, node.grid) {
        auto it = node.grid.get(tile);
        if((tile.x == tile.y)) continue;
        
        auto other = node.grid.get(tile.y, tile.x); // exact opposite
        if(other == it) {
            if(pawn_is_box(it)) {
                count += 3;
            } else {
                count += 1;
            }
        }
    }
    return count;
}
i32 chessboard_value(Mcts_Node &node) {
    const Grid &grid = node.grid;
    i32 count = 0;
    Vector2i tile;
    for_grid(tile, grid) {
        auto it = node.grid.get(tile);
        i32 value = tile.x + tile.y;
        if(value % 2 == 0) {
            if(pawn_is_block(it)) {
                count += 2;
            }
        } else {
            if(pawn_is_box(it)) {
                count += 3;
            } else if(pawn_is_goal(it)) {
                count += 2;
            }
        }
    }
    return count;
}
i32 stripes_value(Mcts_Node &node) {
    i32 count = 0;
    bool swap = false;
    for_range(x, 0, node.grid.width) {
        swap = !swap;
        if(swap) {
            for_range(y, 0, node.grid.height) {
                auto it = node.grid.get(x, y);
                if(pawn_is_box(it) || pawn_is_goal(it)) {
                    count += 1;
                }
            }
        } else {
            for_range(y, 0, node.grid.height) {
                auto it = node.grid.get(x, y);
                if(pawn_is_block(it)) {
                    count += 1;
                }
            }
        }
    }
    return count;
}
i32 group_value(Mcts_Node &node) {
    Vector2i mid = {node.grid.width/2, node.grid.height/2};
    Vector2i tile;
    i32 count = 0;
    for_grid(tile, node.grid) {
        auto pawn = node.grid.get(tile);
        if(tile.x<mid.x && tile.y<mid.y) {
            if(pawn_is_box(pawn)) {
                count += 1;
            }
        } else if(tile.x>mid.x && tile.y>mid.y) {
            if(pawn_is_goal(pawn)) {
                count += 1;
            }
        } else {
            if(pawn_is_block(pawn)) {
                // count += 1;
            }
        }
    }
    return count;
}

// version == 1 or 2
template <int version, bool include_box_on_goal = true>
f64 congestion(Mcts_Node &node, Mcts *tree, const f64 ALPHA = 1.0, const f64 BETA = 1.0, const f64 GAMMA = 1.0) {

    i32 goal_count;
    i32 box_count;
    i32 block_count;

    f64 pc = 0;
    for_range(i, 0, node.grid.get_count()) {
        if(!pawn_is_box(node.grid.get(i))) continue;
        // if(node.first[i] == INVALID_INDEX) continue;

        goal_count = -1; box_count = -1; block_count = 0;

        i32 box_index  = i;
        i32 goal_index = node.second[i];
        assert(pawn_is_box(node.grid.data[box_index]));
        assert(pawn_is_goal(node.grid.data[goal_index]));
        if(box_index == goal_index) continue;
        if constexpr(!include_box_on_goal) {
            if(pawn_is_goal(node.grid.data[box_index])) continue;
        }

        Vector2i box = node.grid.as_tile(box_index);
        Vector2i goal = node.grid.as_tile(goal_index);
        // e.g. {1, 5} - {3, 2} = {-2, 3}
        Vector2i rect = goal - box;
        i32 step_x;
        if(rect.x == 0) {
            step_x = 0;
        } else {
            step_x = (rect.x>0)? 1 : -1;
        }

        i32 step_y;
        if(rect.y == 0) {
            step_y = 0;
        } else {
            step_y = (rect.y>0)? 1 : -1;
        }

        // Add one to each since we start at 0
        i32 count_x = abs(rect.x)+1;
        i32 count_y = abs(rect.y)+1;
        Vector2i tile = box;
        
        // assert(!(box == goal)); // test if move_count > 0?

        for_range(y, 0, count_y) {
            tile.x = box.x;
            for_range(x, 0, count_x) {
                Pawn pawn = node.grid.get(tile.x, tile.y);
                /* if(tree->start_position_tile == tile) {
                    box_count += 1;
                } */ if(pawn_is_goal(pawn)) {
                    goal_count += 1;
                } if(pawn_is_box(pawn)) {
                    box_count += 1;
                } else if(pawn_is_block(pawn)) {
                    block_count += 1;
                }
                tile.x += step_x;
            }
            tile.y += step_y;
        }
        
        if_debug {
            Vector2i step_vec = Vector2i{step_x, step_y};
            assert(tile == goal+step_vec);
            if(!(goal_count >= 0 && box_count >=0)) {
                print(str(node.grid));
                assert(false);
            }
        }
        
        f64 area = f64(count_x) * f64(count_y);
        
        
        f64 s;
        // v2
        if constexpr (version == 2) {
            s = (ALPHA*f64(box_count) + BETA*f64(goal_count))/(GAMMA*(area-f64(block_count)));            
        }
        // v1
        else if constexpr (version == 1) {
            s = ALPHA*f64(box_count) + BETA*f64(goal_count)+GAMMA*f64(block_count);
        }
        pc += s;
        if constexpr (USE_EXAMPLE_LEVELS) {
            println(rect, box, goal, box_count, goal_count, block_count, "/", area, " = ", s);
        }
    }
    return pc;
}

f64 score_node_test(Mcts_Node &node, Mcts *tree, f64 wb, f64 wc, f64 wn, f64 k, f64 ALPHA, f64 BETA, f64 GAMMA) {
    f64 pb = area_score_of_v3(node.grid);
    f64 pc = congestion<2>(node, tree, ALPHA, BETA, GAMMA);
    f64 n = node.box_count;
    f64 score = (wb*pb + wc*pc + wn*n)/k;
    return score * 1.0;
}
f64 score_node(Mcts_Node &node, Mcts *tree) {
    assert(node.flags & MCTS_TERMINAL);
    if(node.box_count<=0) {
        return 0;
    }
    

    f64 wb = 3, wc = 7, wn = 8, k = 55, pb = area_score_of_v2(node.grid);
    f64 pc = congestion<2>(node, tree, 1.9, 0.1, 1.3);
    

    f64 n = node.box_count;
    f64 score = (wb*pb + wc*pc + wn*n)/k;

    // Different secondary scorings for the topology of a level.
    if constexpr (false) {
        f64 val = diagonal_symmetry_value(node);
        score = 0.50*score + (2.0*val)/k;
    }
    if constexpr (false) {
        f64 val = x_symmetry_value(node);
        score = 0.60*score + (2.0*val)/k;
    }    
    if constexpr (false) {
        f64 val = stripes_value(node);
        score = 0.50*score + (2.0*val)/k;
    }
    if constexpr (false) {
        f64 val = group_value(node);
        score = 0.50*score + (2.0*val)/k;
    }
    if constexpr (false) {
        f64 val = chessboard_value(node);
        score = 0.50*score + (2.0*val)/k;
    }
    if constexpr(USE_EXAMPLE_LEVELS) {
        // auto string = str(node.grid); println(string); string.destroy();
        println(pb, " | ",  pc, " | ", n, "\n");
    }
    return score * tree->score_scale;
}

isize get_box_count(Grid &grid) {
    auto count = grid.get_count();
    isize box_count = 0;
    for_range(i, 0, count) {
        if(pawn_is_box(grid.get(i))) {
            box_count += 1;
        }
    }
    return box_count;
}
Level make_level(Grid &grid, i32 box_count, f64 score, Chrono_Clock clock) {
    Level level;
    level.grid = clone_grid(grid);
    level.box_count = box_count;
    level.score = score;
    level.time_stamp = clock;
    return level;
}


#include <algorithm>
void level_sort(Array<Level> &arr) {
    auto p = [](Level &a, Level &b) {
        return a.score < b.score;
    };
    std::sort(arr.data, arr.data + arr.count, p);
}
Array<Level> Mcts::get_level_set(isize count) {
    println("seed used: ", this->seed);
    assert(finished_nodes.count >= 1);
    println("LOOKING THROUGH SETS----------------------------------");

    println("finished count: ", finished_nodes.count);
    count = min(finished_nodes.count, count);
    Array<Level> final;

    f64 max_score = F64_MIN;
    isize min_arg = -1;
    for_range(i, 0, finished_nodes.count) {
        auto it = finished_nodes[i];
        // assert(it->simulation_count == 1);
        if(it.score < 0) {
            crash("bad score sum");
            continue;
        }
        f64 score = it.score;
        if(score > max_score) {
            min_arg = i;
            max_score = score;
        }
    }
    // array_sort(finished_nodes, level_less);
    level_sort(finished_nodes);
    for(isize i = 0; i<count; i+=1) {
        //if(i == min_arg) continue;
        isize idx = (finished_nodes.count - count) + i;
        auto &it = finished_nodes[idx];
        final.add(it);
    }

    println("DONE CREATING LEVEL SET");
    return final;
}

void Mcts::start() {
    #if MT_RANDOM == true
    set_global_random_engine_seed(this->seed);
    #else 
    srand(this->seed);
    #endif
}

f64 get_score_scale(Mcts *tree) {
    return 25.0/tree->area;
}
Mcts *new_mcts(u64 seed, Vector2i size, Vector2i start_position) {
    Mcts mcts = {};
    mcts.depth_soft_cutoff = DEPTH_LOWER_CUTOFF + DEPTH_SOFT_CUTOFF;
    mcts.finished_nodes = make_array<Level>(0, 50);
    mcts.time_start = get_time();
    if(seed == 0) {
        mcts.seed = std::random_device{}();
    } else {
        mcts.seed = seed;
    }
    mcts.size = size;
    mcts.area = size.x * size.y;
    if(BOX_UPPER_CUTOFF < 0) {
        mcts.box_upper_cutoff = ceil(mcts.area/BOX_AREA_CUTOFF);
    } else {
        mcts.box_upper_cutoff = BOX_UPPER_CUTOFF;
    }
    mcts.score_scale = get_score_scale(&mcts);

    Mcts_Node* root = mem_alloc<Mcts_Node>();
    *root = {};

    root->parent = nullptr;
    root->grid = make_grid(size.x, size.y);
    for_range(i, 0, root->grid.get_count()) {
        root->grid.data[i] = Pawn::Block;
    }
    Vector2i mid;
    if (start_position.x < 0) {
        mid = size/2;
    } else {        
        mid = start_position;
        assert(root->grid.in_grid(mid.x, mid.y));
    }

    root->grid(mid.x, mid.y) = Pawn::Empty;

    mcts.root = root;
    mcts.start_position = root->grid.as_index(mid.x, mid.y);
    mcts.start_position_tile = mid;
    assert(mid == root->grid.as_tile(mcts.start_position));
    auto ptr = mem_alloc<Mcts>();
    *ptr = mcts;
    root->pusher = (u8)mcts.start_position;
    return ptr;
}

void root_add_custom_child(Mcts *mcts, Grid &grid, f64 score) {
    auto node = mcts->root;
    Mcts_Node *child = mem_alloc<Mcts_Node>();
    *child = {};
    child->grid = clone_grid(grid);
    child->pusher = node->pusher;

    child->depth = node->depth;
    child->parent = node;
    if(pawn_is_box(child->grid.get(child->pusher))) {
        println(child->grid.as_tile(child->pusher));
        println(str(child->grid));
        assert(false);
    }
    grid_remove_goals_and_pusher(child->grid);
    child->box_count = get_box_count(child->grid);
    /* println(child->box_count);
    println(str(child->grid)); */
    // mcts->finished_nodes.add(make_level(grid, child->box_count, score, get_time()));

    node->children.add(child);
}
Mcts *new_mcts_bootstrap(u64 seed, Vector2i size, Vector2i start_position) {
    Mcts mcts = {};
    mcts.depth_soft_cutoff = DEPTH_LOWER_CUTOFF + DEPTH_SOFT_CUTOFF;
    mcts.finished_nodes = make_array<Level>(0, 50);
    mcts.time_start = get_time();
    mcts.seed = seed;

    mcts.size = size;
    mcts.area = size.x * size.y;
    if(BOX_UPPER_CUTOFF < 0) {
        mcts.box_upper_cutoff = ceil(mcts.area/BOX_AREA_CUTOFF);
    } else {
        mcts.box_upper_cutoff = BOX_UPPER_CUTOFF;
    }
    mcts.score_scale = get_score_scale(&mcts);
    
    

    Mcts_Node *root = mem_alloc<Mcts_Node>();
    *root = {};

    root->parent = nullptr;
    
    root->pusher = Grid::grid_as_index(size, start_position);
    

    root->depth = DEPTH_LOWER_CUTOFF+1;
    root->flags = MCTS_BLOOMED | MCTS_EXPANDED;

    
    mcts.start_position = root->pusher;
    mcts.start_position_tile = start_position;

    mcts.root = root;

    auto ptr = mem_alloc<Mcts>();
    *ptr = mcts;
    return ptr;    
}
void delete_mcts(Mcts* mcts) {
    if(mcts == nullptr) {
        return;
    }
    mcts->root->destroy();
    for_range(i, 0, mcts->finished_nodes.count) {
        mcts->finished_nodes[i].grid.destroy();
    }
    mcts->finished_nodes.destroy();
    mem_free(mcts->root);
    mem_free(mcts);
}


void _get_all_possible_moves(Vector2i tile, Grid &grid, Array<bool> &visited, Array<Move_Info> &moves) {
    for_range(direction, 0, 4) {
        Vector2i v = DIRECTION_TO_VEC[direction];
        Vector2i to = tile + v;
        auto to_idx = grid.as_index(to.x, to.y);
        
        if(!grid.in_grid(to.x, to.y) || visited[to_idx]) continue;
        bool is_empty = (0 == top_layer(grid.get(to_idx)));        
        if(is_empty) {
            visited[to_idx] = true;
            _get_all_possible_moves(to, grid, visited, moves);
        } else {
            bool is_box = pawn_is_box(grid.get(to_idx));
            if(is_box && grid._could_move(tile.x, tile.y, v)) {
                moves.add(Move_Info{(u8)grid.as_index(tile), (u8)direction});
            }
        }
    }
}
// Iterative variant of the the same function.
// There seems to be no performance benefit on the highest optimization level
void _get_all_possible_moves_iterative(Vector2i tile, Grid &grid, Array<bool> &visited, Array<Move_Info> &moves) {
    auto tiles = make_array<Vector2i>(0, grid.get_count());
    tiles.add(tile);
    while(tiles.count > 0) {
        auto tile = tiles[tiles.count-1];
        tiles.count -= 1; 
        for_range(direction, 0, 4) {
            Vector2i v = DIRECTION_TO_VEC[direction];
            Vector2i to = tile + v;
            auto to_idx = grid.as_index(to.x, to.y);
            
            if(!grid.in_grid(to.x, to.y) || visited[to_idx]) continue;
            bool is_empty = (0 == top_layer(grid.get(to_idx)));        
            if(is_empty) {
                visited[to_idx] = true;
                // _get_all_possible_moves(to, grid, visited, moves);
                tiles.add(to);
            } else {
                bool is_box = pawn_is_box(grid.get(to_idx));
                if(is_box && grid._could_move(tile.x, tile.y, v)) {
                    moves.add(Move_Info{(u8)grid.as_index(tile), (u8)direction});
                }
            }
        }
    }
    tiles.destroy();
}



void debug_check_box_count(Mcts_Node &node, const char *msg) {
    if_debug {
        auto box_count = get_box_count(node.grid);
        if ( box_count != node.box_count) {
            println(box_count);
            println(node.box_count);
            println(str(node.grid));
            println(msg);
            if (node.parent) {
                println(node.parent->box_count);
            }
            assert(false);
        }
    }        
}
void remove_impossible_v1(Mcts_Node *node) {
    auto count = node->grid.get_count();
    for_range(i, 0, count) {
        auto pawn = node->grid.get(i);
        
        if(pawn_is_box(pawn)) {
            auto tile = node->grid.as_tile(i);
            for_range(direction, 0, 4) {
                auto d = DIRECTION_TO_VEC[direction];
                auto from = tile - d;
                if(!node->grid.in_grid(from.x, from.y)) {
                    continue;
                }
                auto is_walkable = !pawn_has_collision(node->grid.get(from));
                // check if we could move in direction i.e. push
                // if so then just continue since it is not part
                // of an unrecoverable configuration
                if(is_walkable && node->grid._could_move(from.x, from.y, d)) {
                    goto END_LOOP;
                }
            }            
            assert(pawn_is_box(node->grid.get(tile)));
            // println(str(node->grid));
            node->box_count -= 1;
            node->grid.set(tile.x, tile.y, Pawn::Empty);              
            // println(str(node->grid));
        }
        END_LOOP:
        ((void)0);
    }
}
void remove_impossible_v2(Mcts_Node *node) {
    auto &grid = node->grid;
    // Vector2i tile = {0,0};
    const auto end_width = node->grid.width-1;
    const auto end_height = node->grid.height-1;


    for(i32 y = -1; y < end_height; y += 1) {
        for(i32 x = -1; x < end_width; x += 1) {
            Vector2i box_first = {}, box_second = {};
            Vector2i tile = {0,0};
            bool is_first_box = true;
            i32 box_count = 0;
            i32 block_invalid_count = 0;
            // 2x2 Field
            for_range(dy, 0, 2) {
                tile.y = y + dy; 
                for_range(dx, 0, 2) {
                    tile.x = x + dx;
                    if(!grid.in_grid(tile.x, tile.y)) {
                        block_invalid_count += 1;
                    } else {
                        Pawn pawn = grid.get(tile);
                        if(pawn_is_box(pawn)) {
                            if(is_first_box) {
                                is_first_box = false;
                                box_first = tile;
                            } else {                                    
                                box_second = tile;
                            }
                            box_count += 1;
                        } else if(pawn_is_block(pawn)) {
                            block_invalid_count += 1;
                        }
                    }
                }
            }
            if(box_count == 4 || (block_invalid_count == 3 && box_count == 1)) {
                // println(box_first);  println(str(node->grid));                   
                node->box_count -= 1;
                node->grid.set(box_first.x, box_first.y, Pawn::Empty);                
                break;
            }
            if(box_count == 2 && block_invalid_count == 2) {
                // println(box_first, box_second);  println(str(node->grid));                   
                // make sure they are at the same row or column
                if(manhattan_distance(box_first, box_second) == 1) {
                    node->box_count -= 1;
                    node->grid.set(box_first.x, box_first.y, Pawn::Empty);                
                    break;   
                }
            }
        }
    }

}
#include "mcts.h"

void print_move_infos(Array<Move_Info> &e, Grid &grid) {
	for_range(i, 0, e.count) {
		println(grid.as_tile(e[i].index), " -> ", DIRECTION_TO_VEC[e[i].direction]);
	}
}
void print_node_debug(Mcts_Node &node) {
    println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    println(node.grid.as_tile(node.pusher));
    println(node.grid);
    print_move_infos(node.moves, node.grid);
    println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
}

void print_children(Mcts_Node *node) {
    print("\n child_stats: \n");
    for_range(i, 0, node->children.count) {
        auto child = node->children[i];
        auto score = child->score_sum;
        auto count = child->rollout_count;
        //println()
        print(score, "/", count, "=", score/f64(count), " | ");
    }
    print("\n");
}

void print_node(Mcts_Node &node) {
    println("--------------------------------------");
    for_range(i, 0, node.children.count) {
        Mcts_Node *it = node.children[i];
        print("\n", bool(it->flags & MCTS_EXPANDED), ", ", bool(it->flags & MCTS_SECOND_ACTION) , ", ", it->score_sum, ", ", it->rollout_count, ", ", it->children.count, ", ", it->depth);

        String string = str(it->grid);
        println(string);
        string.destroy();
        print_node(*it);
    }
    println("--------------------------------------");
}

std::ostream &operator<<(std::ostream &os, const Mcts &m) {
    print_node(*m.root);
    return os;
}

void print_scored_nodes(Mcts_Node &node) {
    println("--------------------------------------");
    for_range(i, 0, node.children.count) {
        Mcts_Node *it = node.children[i];
        if(it->score_sum <=0) continue;
        print("\n",bool(it->flags & MCTS_TERMINAL) , ", ", it->score_sum, ", ", it->rollout_count, ", ", (it->children.count>0));

        String string = str(it->grid);
        println(string);
        string.destroy();
        print_scored_nodes(*it);
    }
    println("--------------------------------------");
}
void print_scored_nodes(Mcts &e) {
    print_scored_nodes(*e.root);
}
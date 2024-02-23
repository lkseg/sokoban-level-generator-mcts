#ifndef UCT_ENHANCEMENTS
#define UCT_ENHANCEMENTS

#include "mcts.h"

/*
    Following we have the different ucb implementations which are being used
    in tree_policy. 
    node_* is the function which can be passed as a function pointer.
    Reference Decision_Proc and next_rollout/uct_body.
    Usage: decision_proc in main.
*/

/*
    Default ucb implementation
*/
inline f64 ucb1(f64 score, i64 itotal_n, i64 ilocal_n, const f64 C = 1.0/SQRT2) {
    if(ilocal_n == 0) {
        return F64_MAX;
    }
    f64 total_n = f64(itotal_n);
    f64 local_n = f64(ilocal_n);
    f64 avrg = score/local_n;
    // log is ln in C
    f64 right = 2.0 * C * sqrt( (2.0 * log(total_n))/local_n);

    return avrg + right;
}
inline f64 node_ucb1(Mcts_Node *node) {
    auto u = ucb1(node->score_sum, node->parent->rollout_count, node->rollout_count, UCB1_C);
    return u;
}
/*
    ucb1 tuned implementation
    ucb1 tuned requires a sum of squared scores
*/
#ifdef USE_SQUARED_SUM

inline f64 value_proc(f64 score, f64 squared_score, i64 ilocal_n) {
    f64 local_n = f64(ilocal_n);
    return (squared_score/local_n) - ((score/local_n)*(score/local_n));
}
inline f64 ucb1_tuned(f64 score, f64 squared_score, i64 itotal_n, i64 ilocal_n) {
    if(ilocal_n == 0) {
        return F64_MAX;
    }
    f64 total_n = f64(itotal_n);
    f64 local_n = f64(ilocal_n);
    f64 avrg = score/local_n;
    
    // const f64 C = 3*1.41421356237;

    auto dif = log(total_n)/local_n;
    auto sqrt_d = sqrt(2.0 * dif);
    
    f64 right = sqrt(dif * min(0.25, sqrt_d + value_proc(score, squared_score, ilocal_n)));
    // 2.0 because of the papers(survey) definition
    const f64 C = 2.0 * 4.0 * 1.0/SQRT2;
    return avrg + C * right;
}
inline f64 node_ucb1_tuned(Mcts_Node *node) {
    auto u = ucb1_tuned(node->score_sum, node->squared_score_sum, node->parent->rollout_count, node->rollout_count);
    return u;
}



/*
    ucb-v implementation
*/
inline f64 ucb_v(f64 score, f64 squared_score, i64 itotal_n, i64 ilocal_n) {
    if(ilocal_n == 0) {
        return F64_MAX;
    }
    f64 total_n = f64(itotal_n);
    f64 local_n = f64(ilocal_n);
    f64 avrg = score/local_n;
    f64 variance;
    if(ilocal_n == 1) {
        variance = 0;
    } else {
        auto val = (score*score)/local_n;
        // for precision issues
        if(val > squared_score) {
            variance = 0;
        } else {
            variance = (squared_score - val)/(local_n-1);
        }
    }
    if_debug {
        if(variance < 0) {
            println(squared_score, score, local_n);
        }
    }
    assert(variance >= 0);
    f64 epsilon = 1.0 * log(total_n);

    f64 B = 1.4;
    const f64 C = 2.0 * 8.0 * 1.0/SQRT2;
    // exploration part
    f64 left  = sqrt( (2.0 * epsilon * variance) / local_n);
    f64 right =   C * (3.0 * epsilon * B)        / local_n;
    // println(avrg + left + right);
    return avrg + left + right;
}

inline f64 node_ucb_v(Mcts_Node *node) {
    auto u = ucb_v(node->score_sum, node->squared_score_sum, node->parent->rollout_count, node->rollout_count);
    return u;
}

/*
    Single player mcts implementation
*/
inline f64 sp_mcts(f64 score, f64 squared_score, i64 itotal_n, i64 ilocal_n) {
    if(ilocal_n == 0) {
        return F64_MAX;
    }
    auto _ucb1 = ucb1(score, itotal_n, ilocal_n, UCB1_C);
    f64 local_n = f64(ilocal_n);
    f64 variance;
    if(ilocal_n == 1) {
        variance = 0;
    } else {
        auto val = (score*score)/local_n;
        if(val > squared_score) {
            variance = 0;
        } else {
            variance = (squared_score - val)/(local_n-1);
        }
    }
    assert(variance >= 0);
    f64 possible_deviation = sqrt(variance + SP_MCTS_D/local_n);
    return _ucb1 + possible_deviation;
}
inline f64 node_sp_mcts(Mcts_Node *node) {
    auto u = sp_mcts(node->score_sum, node->squared_score_sum, node->parent->rollout_count, node->rollout_count);
    return u;
}

#endif // USE_SQUARED_SUM

#endif // UCT_ENHANCEMENTS
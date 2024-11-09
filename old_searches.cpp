//
//  old_searches.cpp
//  InvincibleSummer
//
//  Created by Harry Chiu on 11/7/24.
//
#include "engine_core.h"


//ALL THE OLD UNUSED SEARCH FUNCTIONS
//Useful for comparing strength of current engine


uint16_t Engine::search_iterate(int time_limit_ms){
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;
    auto t1 = high_resolution_clock::now();
    uint16_t best_move = pick_move(2);
    for(int search_depth = 3; search_depth < MAX_DEPTH; search_depth++){
        //using aspiration windows, aggressive search window because my engine eval is pretty static right now
        int16_t original_eval = root_search_eval;
        best_move = search_root(original_eval  - 25, original_eval  + 25, search_depth);
        if (root_search_eval <= original_eval  - 25 || original_eval >= root_search_eval + 25){
            best_move = search_root(-DOUBLE_INF, DOUBLE_INF, search_depth);
        }
        //stop the search if we're out of time
        auto t2 = high_resolution_clock::now();
        duration<double, std::milli> ms_double = t2 - t1;
        if(ms_double.count() > time_limit_ms)
            break;
    }
    return best_move;
}


uint16_t Engine::search_root(int alpha, int beta, int depth){
    uint16_t move_list[256];
    int num_moves = board_manager.generate_moves(move_list);
    
    //get stored best move
    uint16_t best_move = 0;
    int16_t score = transposition_table.probe_value(board_manager.zobrist_hash, depth, alpha, beta, &best_move);
    
    //int16_t best_score = -INF;
    for(int i = 0; i < num_moves; i++){
        //order by mvv lva for the rest of the moves
        //if not the first move, or no transposition table hit on best_move
        if(i || !best_move)
            order_moves(move_list, num_moves, i);
        else{
            order_best_first(move_list, num_moves, best_move);
        }
        board_manager.make_move(move_list[i]);
        if(board_manager.is_in_check()){
            board_manager.unmake_move(move_list[i]);
            continue;
        }
        //PVS Search at root, search with null windows on all non pvs moves
        //research if out of bounds
        if(best_move && i == 0){
            score = -negamax_tt_pvs(-beta, -alpha, depth - 1, 0, true);
        }
        else{
            score = -negamax_tt_pvs(-alpha - 1, -alpha, depth - 1, 0, false);
            if(score > alpha){
                score = -negamax_tt_pvs(-beta, -alpha, depth - 1, 0, true);
            }
        }
        board_manager.unmake_move(move_list[i]);
        if(score > alpha){
            best_move = move_list[i];
            alpha = score;
        }
        //print_move(move_list[i]);
        //std::cout << " has a score of: " << score << "\n";
    }
    transposition_table.save_value(board_manager.zobrist_hash, alpha, depth, EXACT_FLAG, best_move);
    root_search_eval = alpha;
    
    //std::cout << "Depth: " << depth << "  Eval: " << alpha << " Move: ";
    //print_move(best_move);
    //std::cout << "\n\n";
    
    return best_move;
}


int16_t Engine::negamax_tt_pvs(int alpha, int beta, int depth, int ply, bool is_pv){
    //reached the end of the search
    if(depth == 0)
        return capture_search(alpha, beta);
    
    if(is_repetition()){
        return CONTEMPT;
    }
    
    //probe eval table, if the depth in table isn't high enough, can still use to order best move
    uint16_t best_move = 0;
    int16_t transposition_eval = transposition_table.probe_value
        (board_manager.zobrist_hash, depth, alpha, beta, &best_move);
    
    //if we do get a hit on the table
    if(transposition_eval != INVALID)
        return transposition_eval;
    
    //generate moves
    uint16_t move_list[256];
    int num_moves = board_manager.generate_moves(move_list);
    int legal_moves = 0;
    int16_t score;
    
    for(int i = 0; i < num_moves; i++){
        //order by mvv lva for the rest of the moves
        //if not the first move, or no transposition table hit on best_move
        if(i || !best_move)
            order_moves(move_list, num_moves, i);
        else{
            order_best_first(move_list, num_moves, best_move);
        }
        board_manager.make_move(move_list[i]);
        if(board_manager.is_in_check()){
            board_manager.unmake_move(move_list[i]);
            continue;
        }
        legal_moves++;
        
        //late move reduction, dont need to waste time searching bad moves
        uint8_t reduction  = 0;
        if(!is_pv && legal_moves > 3 && ply > 3 && (move_list[i] & 0x0f) < 4){
            reduction = 1;
            if(legal_moves > 5)
                reduction = 2;
        }
        int new_depth = std::max(depth - 1 - reduction, 0);
        //Core of the PVS search, null window search on everything but the pv move
        //research if the non pv move is too good
        if(best_move && i == 0){
            score = -negamax_tt_pvs(-beta, -alpha, new_depth, ply + 1, true);
        }
        else{
            score = -negamax_tt_pvs(-alpha - 1, -alpha, new_depth, ply + 1, false);
            if(score > alpha && is_pv){
                score = -negamax_tt_pvs(-beta, -alpha, new_depth, ply + 1, true);
                
                //re-re-search if score exceeds alpha on reduced dep searched on full window
                if(reduction && score > alpha){
                    score = -negamax_tt_pvs(-beta, -alpha, new_depth + reduction, ply + 1, true);
                }
            }
        }
        
        board_manager.unmake_move(move_list[i]);
        
        //where the score cutoffs are performed
        if(score > alpha){
            alpha = score;
            best_move = move_list[i];
            transposition_table.save_value(board_manager.zobrist_hash, alpha, depth, ALPHA_FLAG, best_move);
        }
        if(score >= beta){
            best_move = move_list[i];
            transposition_table.save_value(board_manager.zobrist_hash, beta, depth, BETA_FLAG, best_move);
            return beta;
        }
    }
    if(legal_moves == 0){
        //checkmate
        if(board_manager.position_in_check())
            return -CHECKMATE_SCORE + ply;
        //stalemate
        else
            return CONTEMPT;
    }
    
    //protect against null moves from being saved in TT and screwing us later
    if(best_move)
        transposition_table.save_value(board_manager.zobrist_hash, alpha, depth, EXACT_FLAG, best_move);
    return alpha;
}

int16_t Engine::negamax_tt(int alpha, int beta, int depth, int ply){
    //reached the end of the search
    if(depth == 0)
        return capture_search(alpha, beta);
    int16_t max = -INF;
    uint16_t move_list[256];
    int num_moves = board_manager.generate_moves(move_list);
    int legal_moves = 0;
    
    //probe eval table, if the depth in table isn't high enough, can still use to order best move
    uint16_t best_move = 0;
    uint16_t transposition_eval = transposition_table.probe_value
        (board_manager.zobrist_hash, depth, alpha, beta, &best_move);
    
    //if we do get a hit on the table
    if(transposition_eval != INVALID)
        return transposition_eval;
    int16_t score;
    for(int i = 0; i < num_moves; i++){
        //order by mvv lva for the rest of the moves
        //if not the first move, or no transposition table hit on best_move
        if(i || !best_move)
            order_moves(move_list, num_moves, i);
        else{
            order_best_first(move_list, num_moves, best_move);
        }
        board_manager.make_move(move_list[i]);
        if(board_manager.is_in_check()){
            board_manager.unmake_move(move_list[i]);
            continue;
        }
        legal_moves++;
        score = -negamax_tt(-beta, -alpha, depth - 1, ply + 1);
        board_manager.unmake_move(move_list[i]);
        if(score > max){
            max = score;
            best_move = move_list[i];
            if(score > alpha){
                alpha = score;
                transposition_table.save_value(board_manager.zobrist_hash, alpha, depth, ALPHA_FLAG, best_move);
            }
        }
        if(score >= beta){
            transposition_table.save_value(board_manager.zobrist_hash, beta, depth, BETA_FLAG, best_move);
            return max;
        }
    }
    if(legal_moves == 0){
        //checkmate
        if(board_manager.position_in_check())
            return -CHECKMATE_SCORE + ply;
        //stalemate
        else
            return CONTEMPT;
    }
    if(best_move)
        transposition_table.save_value(board_manager.zobrist_hash, max, depth, EXACT_FLAG, best_move);
    return max;
}

int16_t Engine::negamax(int alpha, int beta, int depth){
    if(depth == 0)
        return get_table_eval();
    int16_t max = -INF;
    uint16_t move_list[256];
    int num_moves = board_manager.generate_moves(move_list);
    int legal_moves = 0;
    for(int i = 0; i < num_moves; i++){
        order_moves(move_list, num_moves, i);
        board_manager.make_move(move_list[i]);
        if(board_manager.is_in_check()){
            board_manager.unmake_move(move_list[i]);
            continue;
        }else{
            legal_moves++;
            int16_t score = -negamax(-beta, -alpha, depth - 1);
            board_manager.unmake_move(move_list[i]);
            if(score > max){
                max = score;
                if(score > alpha){
                    alpha = score;
                }
            } if(score >= beta)
                return max;
        }
    }if(legal_moves == 0)
        return -CHECKMATE_SCORE;
    return max;
}


uint16_t Engine::pick_move_basic(int depth){
    int max = -INF;
    int move_index = -1;
    uint16_t move_list[256];
    int num_moves = board_manager.generate_moves(move_list);
    for(int i = 0; i < num_moves; i++){
        order_moves(move_list, num_moves, i);
        board_manager.make_move(move_list[i]);
        if (!board_manager.is_in_check()){
            int score = -negamax(-DOUBLE_INF, DOUBLE_INF, depth - 1);
            if(score > max){
                max = score;
                move_index = i;
            }
        }
        board_manager.unmake_move(move_list[i]);
    }
    //std::cout << " " << max << " ";
    return move_list[move_index];
}

uint16_t Engine::pick_move(int depth){
    int max = -INF;
    int move_index = -1;
    uint16_t move_list[256];
    int num_moves = board_manager.generate_moves(move_list);
    for(int i = 0; i < num_moves; i++){
        order_moves(move_list, num_moves, i);
        board_manager.make_move(move_list[i]);
        if (!board_manager.is_in_check()){
            int score = -negamax_tt(-INF, INF, depth - 1, 0);
            //print_move(move_list[i]);
            //std::cout << " :" << score << "\n";
            if(score > max){
                max = score;
                move_index = i;
            }
        }
        board_manager.unmake_move(move_list[i]);
    }
    //std::cout << " " << max << " ";
    transposition_table.save_value(board_manager.zobrist_hash, max, depth, EXACT_FLAG, move_list[move_index]);
    root_search_eval = max;
    return move_list[move_index];
}

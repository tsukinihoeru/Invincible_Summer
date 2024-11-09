//
//  search.cpp
//  Invincible_Summer
//
//  Created by Harry Chiu on 10/26/24.
//

#include "engine_core.h"


int16_t piece_values[8] = {0, 0, 82,  365, 337, 477, 1025,  0};

uint16_t Engine::search_iterate_null(int time_limit_ms){
    //mark the start of our search
    start_search_time = std::chrono::high_resolution_clock::now();
    search_time_limit = time_limit_ms;
    
    //old values in history heuristics need to be reduced to prevent saturation
    age_heuristics();
    
    //Do an initial search with depth 3 to generate a rough guess at the evaluation
    uint16_t best_move = pick_move(2);
    
    for(int search_depth = 3; search_depth < MAX_DEPTH; search_depth++){
        
        //using aspiration windows, aggressive search window because my engine eval is pretty static right now
        int16_t original_eval = root_search_eval;
        best_move = search_root_null(original_eval  - 25, original_eval  + 25, search_depth);
        if (root_search_eval <= original_eval  - 25 || original_eval >= root_search_eval + 25){
            best_move = search_root_null(-DOUBLE_INF, DOUBLE_INF, search_depth);
        }
        
        //stop the search if we're out of time
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - start_search_time;
        if(ms_double.count() > time_limit_ms)
            break;
    }
    if(nodes > 10){
        std::cout << "henlo";
    }
    return best_move;
}


uint16_t Engine::search_root_null(int alpha, int beta, int depth){
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
            score = -negamax_tt_pvs_null(-beta, -alpha, depth - 1, 0, true, true);
        }
        else{
            score = -negamax_tt_pvs_null(-alpha - 1, -alpha, depth - 1, 0, false, true);
            if(score > alpha){
                score = -negamax_tt_pvs_null(-beta, -alpha, depth - 1, 0, true, true);
            }
        }
        board_manager.unmake_move(move_list[i]);
        if(score > alpha){
            best_move = move_list[i];
            alpha = score;
        }
        //stop the search if we're out of time
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - start_search_time;
        if(ms_double.count() > search_time_limit)
            break;
       // print_move(move_list[i]);
       //  std::cout << " has a score of: " << score << "\n";
    }
    transposition_table.save_value(board_manager.zobrist_hash, alpha, depth, EXACT_FLAG, best_move);
    root_search_eval = alpha;
    
    //std::cout << "Depth: " << depth << "  Eval: " << alpha << " Move: ";
    //print_move(best_move);
    //std::cout << " NULL\n\n";
    return best_move;
}



int16_t Engine::negamax_tt_pvs_null(int alpha, int beta, int depth, int ply, bool is_pv, bool can_null){
    
    //Step 1. check if we reached the end of the search
    //if so, continue to the capture search
    if(depth == 0)
        return capture_search(alpha, beta);
    
    //Step 2. detect if position is a repeat
    if(is_repetition()){
        return CONTEMPT;
    }
    
    
    /*
    Step 3. Null move pruning, see if any moves are so shit we can immediately skip evaluating it
    misses zugzwang, can't use in endgames
    criterias are
     1. can_null flag is set (avoids 2 null moves in a row)
     2. not a pv
     3. not an endgame (gamestate > 6)
     4. not in check right now
     5. static eval is greater than beta
    */
    
    if(can_null && depth > 3 && !is_pv && get_game_state() > 6 && !board_manager.position_in_check() &&
       get_table_eval() >= beta){
        board_manager.make_null_move();
        int reduction = 2;
        if(depth > 6)
            reduction = 3;
        int16_t score = -negamax_tt_pvs_null(-beta, -beta + 1, depth - reduction - 1, ply, false, false);
        board_manager.unmake_null_move();
        //our position is god tier even after making a null move
        if(score >= beta){
            return beta;
        }
    }
    
    
    //Step 4. probe eval table, If we searched this position before at a greater depth, don't need to search again
    //Even if the depth in table isn't high enough, we can still use it to order best move
    uint16_t best_move = 0;
    int16_t transposition_eval = transposition_table.probe_value
        (board_manager.zobrist_hash, depth, alpha, beta, &best_move);
    //if we do get a hit on the table
    if(transposition_eval != INVALID)
        return transposition_eval;
    
    //Step 5. generate moves, initialize necessary variables
    uint16_t move_list[256];
    int num_moves = board_manager.generate_moves(move_list);
    int legal_moves = 0;
    int16_t score;
    //This stores all the quiet moves that failed to raise alpha
    //will be given a negative in history heuristics if another move causes a beta cutoff
    uint16_t unimpressive_quiet_moves[256];
    uint16_t quiet_index = 0;
    
    //Now we loop through all the moves
    for(int i = 0; i < num_moves; i++){
        //Step 6. Order all moves
        //Best move is first (found by the transposition table
        //Then all captures are sorted by mvv lva
        //Quiet moves are then sorted by history heuristics
        if(i || !best_move)
            order_moves(move_list, num_moves, i);
        else{
            order_best_first(move_list, num_moves, best_move);
        }
        
        //Step 7. Make the move, check if the move is legal
        board_manager.make_move(move_list[i]);
        if(board_manager.is_in_check()){
            board_manager.unmake_move(move_list[i]);
            continue;
        }
        legal_moves++;
        
        //Step 8. late move reduction, idea is that moves late in the search cycle are bad
        //the later the move occurs, the further we reduce the depth
        uint8_t reduction  = 0;
        if(!is_pv && legal_moves > 3 && ply > 3 && (move_list[i] & 0x0f) < 4){
            reduction = 1;
            if(legal_moves > 5)
                reduction = 2;
        }
        int new_depth = std::max(depth - 1 - reduction, 0);
        
        /*
        Step 9. PVS Search
        We only search the principle variation from the previous depth with a full window
        All other moves are searched with a null (width 1) window
        If it turns out that the move is better than the principle move, research the new move as the principle move
        */
        if(best_move && i == 0){
            score = -negamax_tt_pvs_null(-beta, -alpha, new_depth, ply + 1, true, true);
        }
        else{
            score = -negamax_tt_pvs_null(-alpha - 1, -alpha, new_depth, ply + 1, false, true);
            if(score > alpha && is_pv){
                score = -negamax_tt_pvs_null(-beta, -alpha, new_depth, ply + 1, true, true);
                //re-re-search if score exceeds alpha on reduced dep searched on full window
                if(reduction && score > alpha){
                    score = -negamax_tt_pvs_null(-beta, -alpha, new_depth + reduction, ply + 1, true, true);
                }
            }
        }
        board_manager.unmake_move(move_list[i]);
        
        //Step 10. Perform search cutoffs (alpha-beta pruning)
        //If the score is greater than the current best possible score (alpha)
        //We raise alpha and have found a new best move
        //Otherwise we've found an unimpressive quiet move, which we append to our list of unimpressive quiet moves
        if(score > alpha){
            alpha = score;
            best_move = move_list[i];
            transposition_table.save_value(board_manager.zobrist_hash, alpha, depth, ALPHA_FLAG, best_move);
        }else if(!is_pv){
            unimpressive_quiet_moves[quiet_index] = move_list[i];
            quiet_index++;
        }
        //If the current score is higher than beta, then we have found a killer move
        //We save the move to history heuristics and the transposition table
        //Then return beta, since our current position is too good
        if(score >= beta){
            best_move = move_list[i];
            transposition_table.save_value(board_manager.zobrist_hash, beta, depth, BETA_FLAG, best_move);
            //save the quiet move to history heuristics
            //penalize all the unimpressive quiet moves that came before it
            if(!(best_move & 0x04)){
                update_heuristics(board_manager.mailbox[(best_move >> 10) & 0x3f], (best_move >> 4) & 0x3f, depth * depth);
                for(int j = 0; j < quiet_index; j++){
                    update_heuristics(board_manager.mailbox[(unimpressive_quiet_moves[j] >> 10) & 0x3f], (unimpressive_quiet_moves[j] >> 4) & 0x3f, -depth * depth / 2);
                }
            }
            return beta;
        }
    }
    
    //Step 10. Mate/Stalemate detection
    //We decrease the value of checkmates by how far in the future they are
    //Therefore the engine prefers the fastest mate
    if(legal_moves == 0){
        //checkmate
        if(board_manager.position_in_check())
            return -CHECKMATE_SCORE + ply;
        //stalemate
        else
            return CONTEMPT;
    }
    
    //Step 11. Save the best move into the TT as an exact value, since no cutoffs occured atp, then return alpha
    //protect against null moves from being saved in TT and screwing us later
    if(best_move)
        transposition_table.save_value(board_manager.zobrist_hash, alpha, depth, EXACT_FLAG, best_move);
    return alpha;
}

int16_t Engine::capture_search(int alpha, int beta){
    //let the search stabalize on the static score
    //if the static score is better than the high cutoff, any captures will only make the position
    int static_score = get_table_eval();
    if(static_score >= beta)
        return static_score;
    if(alpha < static_score)
        alpha = static_score;
    uint16_t move_list[100];
    int move_num = board_manager.generate_captures(move_list);
    for(int i = 0; i < move_num; i++){
        order_moves(move_list, move_num, i);
        int captured_piece_value = piece_values[board_manager.mailbox[(move_list[i] >> 4) & 0x3f] >> 1];
        
        //delta cutoff, turn off for endgames
        if ((static_score + captured_piece_value + 200 < alpha) && (move_list[i] & 0x0f) == CAPTURE_FLAG){
            if(get_game_state() > 6)
                continue;
        }
        
        board_manager.make_move(move_list[i]);
        if(board_manager.is_in_check()){
            board_manager.unmake_move(move_list[i]);
            continue;
        }
        int score = -capture_search(-beta, -alpha);
        board_manager.unmake_move(move_list[i]);
        if(score > alpha){
            if(score >= beta)
                return beta;
            alpha = score;
        }
    }return alpha;
}

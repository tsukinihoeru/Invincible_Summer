//
//  move_ordering.cpp
//  InvincibleSummer
//
//  Created by Harry Chiu on 11/7/24.
//

#include "engine_core.h"

void Engine::update_heuristics(int piece, int dest, int bonus){
    int clamped_bonus = std::clamp(bonus, -MAX_HISTORY_SCORE, MAX_HISTORY_SCORE);
    history_heuristics[piece][dest] += clamped_bonus - abs(clamped_bonus) * history_heuristics[piece][dest] / MAX_HISTORY_SCORE;
}

void Engine::age_heuristics(){
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 64; j++){
            history_heuristics[i][j] /= 8;
        }
    }
}

inline int Engine::get_move_score(uint16_t move){
    int source = (move >> 10) & 0x3f;
    int dest = (move >> 4) & 0x3f;
    if(move & 0x04){
        int lookup_index = (board_manager.mailbox[source] >> 1) + 8 * (board_manager.mailbox[dest] >> 1);
        return mvv_lva[lookup_index] + MAX_HISTORY_SCORE;
    }
    return history_heuristics[board_manager.mailbox[source]][dest];
}

void Engine::order_best_first(uint16_t * move_list, int num_moves, uint16_t best_move){
    for(int i = 0; i < num_moves; i++){
        if(move_list[i] == best_move){
            uint16_t temp = move_list[i];
            move_list[i] = move_list[0];
            move_list[0] = temp;
            return;
        }
    }
}

void Engine::order_moves(uint16_t *move_list, int num_moves, int start_index){
    for(int i = start_index; i < num_moves; i++){
        if(get_move_score(move_list[i]) > get_move_score(move_list[start_index])){
            uint16_t temp = move_list[i];
            move_list[i] = move_list[start_index];
            move_list[start_index] = temp;
        }
    }
}

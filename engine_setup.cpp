//
//  engine_setup.cpp
//  Invincible_Summer
//
//  Created by Harry Chiu on 10/25/24.
//

#include "engine_core.h"
#include <random>

void Engine::print_board(){
    board_manager.print_board();
}

bool Engine::is_checkmate(){
    uint16_t move_list[256];
    int length = board_manager.generate_moves(move_list);
    for(int i = 0; i < length; i++){
        board_manager.make_move(move_list[i]);
        if(!board_manager.is_in_check()){
            board_manager.unmake_move(move_list[i]);
            return false;
        }board_manager.unmake_move(move_list[i]);
    }
    return true;
}

uint16_t Engine::pick_random_move(){
    std::vector<uint16_t> legal_moves;
    uint16_t move_list[256];
    int length = board_manager.generate_moves(move_list);
    for(int i = 0; i < length; i++){
        board_manager.make_move(move_list[i]);
        if(!board_manager.is_in_check()){
            legal_moves.push_back(move_list[i]);
        }board_manager.unmake_move(move_list[i]);
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, legal_moves.size() - 1);
    int a = distrib(gen);
    return legal_moves[a];
}

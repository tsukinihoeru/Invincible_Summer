//
//  engine_core.h
//  Invincible_Summer
//
//  Created by Harry Chiu on 10/25/24.
//
#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H
#include "bitboard_gen.h"
#define DOUBLE_INF 0x1002
#define INF 0x1001
#define CHECKMATE_SCORE 0x1000
#define MAX_DEPTH 30

void print_move(uint16_t move);

class Engine{
public:
    int nodes = 0;
    
    //evaluation stored here after every search, used for aspiration window
    int16_t root_search_eval = 0;
    
    //Constructor
    Engine() : board_manager("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"), transposition_table(128){};
    Engine(std::string fen) : board_manager(fen), transposition_table(128){};
    void set_board(std::string fen){board_manager.set_board(fen);}
    
    //Search section, in search.cpp
    uint16_t search_iterate(int time_limit_ms);
    uint16_t pick_move(int depth);
    uint16_t pick_move_basic(int depth);
    uint16_t pick_random_move();
    uint16_t search_root(int alpha, int beta, int depth);
    int16_t negamax(int alpha, int beta, int depth);
    int16_t negamax_tt(int alpha, int beta, int depth, int ply);
    int16_t negamax_tt_pvs(int alpha, int beta, int depth, int ply, bool is_pv);
    int16_t capture_search(int alpha, int beta);
    int dumb_search(int depth);
    
    //order moves for search
    void order_moves(uint16_t *move_list, int num_moves, int start_index);
    void order_best_first(uint16_t *move_list, int num_moves, uint16_t best_move);
    inline int get_move_score(uint16_t move);
    
    bool is_checkmate(); //don't actually use in search functions, for debugging only
    
    //Evaluation section, specified in evaluation.cpp
    int16_t get_table_eval();
    int16_t get_game_state();
    
    //boilerplate, bugfixing, boring in engine_setup.cpp
    void print_board();
    Bitboard_Gen board_manager;
    Transposition_Table transposition_table;
    
    
    //get piece values by bitshifting right 1 from mailbox
    //do victim_piece_value * 8 + attacker_piece_value to get index
    //returns 0 if victim_piece_value is 0 (empty)
    const int mvv_lva [64] = {
        0, 0, 0, 0, 0, 0, 0, 0, //Padding
        0, 0, 0, 0, 0, 0, 0, 0, //Padding
        0, 0, 15, 13, 14, 12, 11, 10, // Victim P, attacker NULL NULL P B N R Q K
        0, 0, 35, 33, 34, 32, 31, 10, // Victim B, attacker NULL NULL P B N R Q K
        0, 0, 25, 23, 24, 22, 21, 20, // Victim N, attacker NULL NULL P B N R Q K
        0, 0, 45, 43, 44, 42, 41, 40, // Victim R, attacker NULL NULL P B N R Q K
        0, 0, 55, 53, 54, 52, 51, 50, // Victim Q, attacker NULL NULL P B N R Q K
        0, 0, 0, 0, 0, 0, 0, 0
    };
};

#endif

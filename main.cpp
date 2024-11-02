//
//  main.cpp
//  Invincible_Summer
//
//  Created by Harry Chiu on 10/24/24.
//

#include <iostream>
#include "engine_core.h"

// Define PLAY_AS_WHITE to choose whether you play as White or Black
// Uncomment the line below to play as White, or comment it out to play as Black
#define PLAY_AS_WHITE

uint16_t string_to_move(std::string str){
    uint16_t source = ((int)str[0] - (int) 'a') + ((int) str[1] - (int) '1') * 8;
    uint16_t dest = ((int)str[2] - (int) 'a') + ((int) str[3] - (int) '1') * 8;
    uint16_t flag = (int) str[4] - (int) '0';
    uint16_t move = (source << 10) | (dest << 4) | flag;
    return move;
}

int main(int argc, const char * argv[]) {
    Engine engine;
    //Engine engine1;
    /*
    print_move(engine.search_iterate(10000));
    for(int i = 0; i < 9; i++){
        std::cout << "\n";
        uint16_t move = engine.transposition_table.get_best_move(engine.board_manager.zobrist_hash);
        print_move(move);
        engine.board_manager.make_move(move);
       
    }
    */
    
    for(int k = 0; k < 1; k++){
        //engine.board_manager.set_board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq");
        for(int i = 0; i < 100; i++){
            if (engine.is_checkmate()) {
                std::cout << (i % 2 == 0 ? "Black Wins\n" : "White Wins\n");
                break;
            }
            
#ifdef PLAY_AS_WHITE
            // If playing as White, player goes first (even turns)
            if (i % 2 == 0) {
#else
            // If playing as Black, engine goes first (even turns)
            if (i % 2 != 0) {
#endif
                std::string s;
                std::cout << "\nEnter move: ";
                std::cin >> s;
                engine.board_manager.make_move(string_to_move(s));
                std::cout << "\n";
                engine.print_board();
            } else { 
                // Engine's move
                uint16_t engine_move = engine.search_iterate(10000);
                print_move(engine_move);
                std::cout << " ";
                engine.board_manager.make_move(engine_move);
            }
            std::cout << "\n";
            
            /*
            uint16_t random_move = engine1.pick_move(7);
            print_move(random_move);
            std::cout << " ";
            engine.board_manager.make_move(random_move);
            engine1.board_manager.make_move(random_move);
             */
            //engine.print_board();
        }
    }
    return 0;
}

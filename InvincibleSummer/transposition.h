//
//  transposition.h
//  Invincible_Summer
//
//  Created by Harry Chiu on 10/27/24.
//
#include <cstdint>

#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#define INVALID 0x1111
#define EXACT_FLAG 1
#define ALPHA_FLAG 2
#define BETA_FLAG 3

struct table_entry{
    uint64_t zobrist_hash;
    int16_t evaluation;
    uint8_t depth;
    uint8_t flag;
    uint16_t best_move;
    table_entry(){
        zobrist_hash = 0;
        evaluation = 0;
    }table_entry(uint64_t zhash, int64_t eval, uint8_t tdepth, uint8_t tflag, uint16_t b_move){
        zobrist_hash = zhash;
        evaluation = eval;
        depth = tdepth;
        flag = tflag;
        best_move = b_move;
    }
};

class Transposition_Table{
public:
    uint64_t table_size;
    table_entry * table;
    Transposition_Table(int sizeMBytes){
        uint64_t sizeBytes = sizeMBytes*1024*1024;
        std::size_t entrySize = sizeof(table_entry);
        uint64_t nEntries = sizeBytes / entrySize;
        table_size = nEntries;
        table = new table_entry[table_size];
        //std::cout << table_size;
    }
    uint16_t probe_value(uint64_t z_hash, uint8_t depth, int alpha, int beta, uint16_t * best_move){
        uint64_t index = z_hash % table_size;
        if(table[index].zobrist_hash == z_hash){
            *best_move = table[index].best_move;
            if(table[index].depth > depth){
                if(table[index].flag == EXACT_FLAG)
                    return table[index].evaluation;
                else if(table[index].flag == ALPHA_FLAG && table[index].evaluation <= alpha)
                    return alpha;
                else if(table[index].flag == BETA_FLAG && table[index].evaluation >= beta)
                    return beta;
            }
        }
        return INVALID;
    }
    uint16_t get_best_move(uint64_t z_hash){
        uint64_t index = z_hash % table_size;
        if(table[index].zobrist_hash == z_hash){
            std::cout<< "Table eval: " << table[index].evaluation << " move: ";
            return table[index].best_move;
        }
        std::cout<< "ERROR CANT FIND MOVE \n" ;
        return 0;
    }
    void save_value(uint64_t z_hash, int16_t eval, uint8_t depth, uint8_t flag, uint16_t best_move){
        if(!best_move) {
            std::cout << "trying to save null move ";
            return;
        }
        uint64_t index = z_hash % table_size;
        if(table[index].zobrist_hash == z_hash){
            //something is storing null moves into my transposition table
            //and i really dont like it, but heres a bandaid solution for now
            if(table[index].depth > depth && table[index].best_move)
                return;
        }
        table[z_hash % table_size] = table_entry(z_hash, eval, depth, flag, best_move);
    }
};

#endif

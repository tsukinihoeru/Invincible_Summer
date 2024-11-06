//
//  bitboard_gen.h
//  Bitboard_Movegen
//
//  Created by Harry Chiu on 10/17/24.
//
#include <iostream>
#include <string>
#include <cstdint>
#include "utility.h"
#include "transposition.h"
#include <unordered_map>

#ifndef BITBOARD_GEN
#define BITBOARD_GEN

#if defined(__APPLE__)
    #include <libkern/OSByteOrder.h>
    #define bswap_32(x) OSSwapInt32(x)
    #define bswap_64(x) OSSwapInt64(x)
#elif defined(__linux__)
    #include <endian.h>
    #define bswap_32(x) __bswap_32(x)
    #define bswap_64(x) __bswap_64(x)
#elif defined(_WIN32)
    #include <stdlib.h>
    #define bswap_32(x) _byteswap_ulong(x)
    #define bswap_64(x) _byteswap_uint64(x)
#else
    #error "Byte swapping functions are not defined for this platform."
#endif

#define WHITE 0
#define BLACK 1
#define PAWN_BOARD 2
#define BISHOP_BOARD 3
#define KNIGHT_BOARD 4
#define ROOK_BOARD 5
#define QUEEN_BOARD 6
#define KING_BOARD 7

#define QUIET_FLAG 0
#define DOUBLE_PAWN_PUSH_FLAG 1
#define KINGSIDE_CASTLE_FLAG 2
#define QUEENSIDE_CASTLE_FLAG 3
#define CAPTURE_FLAG 4
#define EN_PASSANT_FLAG 5
#define KNIGHT_PROMO_FLAG 8
#define BISHOP_PROMO_FLAG 9
#define ROOK_PROMO_FLAG 10
#define QUEEN_PROMO_FLAG 11
#define KNIGHT_PROMO_CAP_FLAG 12
#define BISHOP_PROMO_CAP_FLAG 13
#define ROOK_PROMO_CAP_FLAG 14
#define QUEEN_PROMO_CAP_FLAG 15

#define WKS_CASTLING_RIGHTS 8
#define WQS_CASTLING_RIGHTS 4
#define BKS_CASTLING_RIGHTS 2
#define BQS_CASTLING_RIGHTS 1

#define U64 uint64_t

struct game_state{
    uint8_t castling_rights;
    int captured; //first bit is the side, rest is piece type
    int ep_target; //no fucking clue
    game_state(uint16_t castling, int cap, int ep){
        castling_rights = castling;
        captured = cap;
        ep_target = ep;
    }
    game_state(){
        castling_rights = 0;
        captured = 0;
        ep_target = 0;
    }
};

struct zobrist_struct {
    U64 piecesquare[16][64];
    U64 color;
    U64 castling[16];
    U64 ep_squares[40];
};

class Bitboard_Gen{
    
public:
    int mailbox[64]; //first bit represents color, rest represent piece type
    U64 bitboards[8];
    U64 zobrist_hash = 0; //current zobrist hash of position
    game_state game_history[200];
    int current_side = WHITE;
    int ply = 0;
    zobrist_struct zobrist_keys;
    
    Bitboard_Gen();
    Bitboard_Gen(std::string fen);
    void init_zobrist_keys();
    void set_board(std::string fen);
    void clear_board();
    void move_piece(int source, int dest);
    void add_piece(int piece_color, int piece_type, int square_index);
    void add_piece(int piece, int square_index);
    void remove_piece(int source);
    
    //to test move legality, the king can be captured
    bool is_in_check();
    //The usual one, side to move needs to escape check
    bool position_in_check();
    
    int generate_moves(uint16_t * move_list);
    int generate_moves(uint16_t * move_list, bool side);
    int generate_captures(uint16_t * move_list);
    U64 generate_attacked_squares(bool side);
    U64 hyp_quint(int source, U64 occupied, U64 mask);
    U64 hyp_quint_horiz(int source, U64 occupied, U64 mask);
    inline uint16_t * add_quiet_moves(int source, U64 dests, uint16_t * move_list);
    inline uint16_t * add_capture_moves(int source, U64 dests, uint16_t * move_list);
    inline uint16_t * add_promo_moves(int source, int dest, uint16_t * move_list);
    inline uint16_t * add_promo_cap_moves(int source, int dest, uint16_t * move_list);
    
    void make_move(uint16_t move);
    void unmake_move(uint16_t move);
    void make_null_move();
    void unmake_null_move();
    U64 perft(int depth);
    
    bool check_consistency();
    void print_board();
    void print_bit_boards();
    void print_u64(U64 bitboard);
    //returns square index of lsb and removes lsb from bitboard
    inline int pop_lsb(U64 * bitboard){
        int lsb_index = get_square_index(*bitboard);
        (*bitboard) = (*bitboard) & (*bitboard - 1);
        return lsb_index;
    };
    //returns square index of lsb
    constexpr int get_square_index(U64 bitboard){
        return index_debruges64[(((bitboard) ^ ((bitboard) - 1)) * debruges) >> 58];
    };
private:
    
    std::unordered_map<char, int> fen_char_to_piece_type = {{'p', PAWN_BOARD}, {'b', BISHOP_BOARD}, {'n', KNIGHT_BOARD},
        {'r', ROOK_BOARD}, {'q', QUEEN_BOARD}, {'k', KING_BOARD}};
    
    const int source_to_rank[64]{
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7,
    };
    const int source_to_file[64]{
        0, 1, 2, 3, 4, 5, 6, 7,
        0, 1, 2, 3, 4, 5, 6, 7,
        0, 1, 2, 3, 4, 5, 6, 7,
        0, 1, 2, 3, 4, 5, 6, 7,
        0, 1, 2, 3, 4, 5, 6, 7,
        0, 1, 2, 3, 4, 5, 6, 7,
        0, 1, 2, 3, 4, 5, 6, 7,
        0, 1, 2, 3, 4, 5, 6, 7
    };
    const int source_to_diagonal[64]{
        7, 6, 5, 4, 3, 2, 1, 0,
        8, 7, 6, 5, 4, 3, 2, 1,
        9, 8, 7, 6, 5, 4, 3, 2,
        10, 9, 8, 7, 6, 5, 4, 3,
        11, 10, 9, 8, 7, 6, 5, 4,
        12, 11, 10, 9, 8, 7, 6, 5,
        13, 12, 11, 10, 9, 8, 7, 6,
        14, 13, 12, 11, 10, 9, 8, 7
    };
    const int source_to_antidiagonal[64]{
        0, 1, 2, 3, 4, 5, 6, 7,
        1, 2, 3, 4, 5, 6, 7, 8,
        2, 3, 4, 5, 6, 7, 8, 9,
        3, 4, 5, 6, 7, 8, 9, 10,
        4, 5, 6, 7, 8, 9, 10, 11,
        5, 6, 7, 8, 9, 10, 11, 12,
        6, 7, 8, 9, 10, 11, 12, 13,
        7, 8, 9, 10, 11, 12, 13, 14
    };
    
    //0 for 1st rank, 7 for 8th
    const U64 rank_masks[8]{
        0x00000000000000ff,
        0x000000000000ff00,
        0x0000000000ff0000,
        0x00000000ff000000,
        0x000000ff00000000,
        0x0000ff0000000000,
        0x00ff000000000000,
        0xff00000000000000,
    };
    //0 for a file, 7 for h
    const U64 file_masks[8]{
        0x0101010101010101,
        0x0202020202020202,
        0x0404040404040404,
        0x0808080808080808,
        0x1010101010101010,
        0x2020202020202020,
        0x4040404040404040,
        0x8080808080808080
    };
    
    const U64 diagonal_masks[15] = {
        0x80, 0x8040, 0x804020,
        0x80402010, 0x8040201008, 0x804020100804,
        0x80402010080402, 0x8040201008040201, 0x4020100804020100,
        0x2010080402010000, 0x1008040201000000, 0x804020100000000,
        0x402010000000000, 0x201000000000000, 0x100000000000000
    };

    const U64 antidiagonal_masks[15] = {
        0x1, 0x102, 0x10204,
        0x1020408, 0x102040810, 0x10204081020,
        0x1020408102040, 0x102040810204080, 0x204081020408000,
        0x408102040800000, 0x810204080000000, 0x1020408000000000,
        0x2040800000000000, 0x4080000000000000, 0x8000000000000000
    };
    
    //big fat lookup table for king attacks
    const U64 king_move_lookup[64]{
        0x302, 0x705, 0xe0a, 0x1c14,
        0x3828, 0x7050, 0xe0a0, 0xc040,
        0x30203, 0x70507, 0xe0a0e, 0x1c141c,
        0x382838, 0x705070, 0xe0a0e0, 0xc040c0,
        0x3020300, 0x7050700, 0xe0a0e00, 0x1c141c00,
        0x38283800, 0x70507000, 0xe0a0e000, 0xc040c000,
        0x302030000, 0x705070000, 0xe0a0e0000, 0x1c141c0000,
        0x3828380000, 0x7050700000, 0xe0a0e00000, 0xc040c00000,
        0x30203000000, 0x70507000000, 0xe0a0e000000, 0x1c141c000000,
        0x382838000000, 0x705070000000, 0xe0a0e0000000, 0xc040c0000000,
        0x3020300000000, 0x7050700000000, 0xe0a0e00000000, 0x1c141C00000000,
        0x38283800000000, 0x70507000000000, 0xe0a0e000000000, 0xc040C000000000,
        0x302030000000000, 0x705070000000000, 0xe0a0e0000000000, 0x1c141C0000000000,
        0x3828380000000000, 0x7050700000000000, 0xe0a0e00000000000, 0xc040C00000000000,
        0x203000000000000, 0x507000000000000, 0xa0e000000000000, 0x141C000000000000,
        0x2838000000000000, 0x5070000000000000, 0xa0e0000000000000, 0x40C0000000000000
    };
    //big fat lookup table for knight attacks
    const U64 knight_move_lookup[64] = {
        0x20400, 0x50800, 0xa1100, 0x142200,
        0x284400, 0x508800, 0xa01000, 0x402000,
        0x2040004, 0x5080008, 0xa110011, 0x14220022,
        0x28440044, 0x50880088, 0xa0100010, 0x40200020,
        0x204000402, 0x508000805, 0xa1100110a, 0x1422002214,
        0x2844004428, 0x5088008850, 0xa0100010a0, 0x4020002040,
        0x20400040200, 0x50800080500, 0xa1100110a00, 0x142200221400,
        0x284400442800, 0x508800885000, 0xa0100010a000, 0x402000204000,
        0x2040004020000, 0x5080008050000, 0xa1100110a0000, 0x14220022140000,
        0x28440044280000, 0x50880088500000, 0xa0100010a00000, 0x40200020400000,
        0x204000402000000, 0x508000805000000, 0xa1100110a000000, 0x1422002214000000,
        0x2844004428000000, 0x5088008850000000, 0xa0100010a0000000, 0x4020002040000000,
        0x400040200000000, 0x800080500000000, 0x1100110a00000000, 0x2200221400000000,
        0x4400442800000000, 0x8800885000000000, 0x100010a000000000, 0x2000204000000000,
        0x4020000000000, 0x8050000000000, 0x110a0000000000, 0x22140000000000,
        0x44280000000000, 0x0088500000000000, 0x0010a00000000000, 0x20400000000000
    };
    
    const U64 pawn_capture_lookup[2][64]{ //0 for white, black for 1
        {
            0x200, 0x500, 0xa00, 0x1400,
            0x2800, 0x5000, 0xa000, 0x4000,
            0x20000, 0x50000, 0xa0000, 0x140000,
            0x280000, 0x500000, 0xa00000, 0x400000,
            0x2000000, 0x5000000, 0xa000000, 0x14000000,
            0x28000000, 0x50000000, 0xa0000000, 0x40000000,
            0x200000000, 0x500000000, 0xa00000000, 0x1400000000,
            0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000,
            0x20000000000, 0x50000000000, 0xa0000000000, 0x140000000000,
            0x280000000000, 0x500000000000, 0xa00000000000, 0x400000000000,
            0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000,
            0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000,
            0x200000000000000, 0x500000000000000, 0xa00000000000000, 0x1400000000000000,
            0x2800000000000000, 0x5000000000000000, 0xa000000000000000, 0x4000000000000000,
            0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0,
        },{
            0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0,
            0x2, 0x5, 0xa, 0x14,
            0x28, 0x50, 0xa0, 0x40,
            0x200, 0x500, 0xa00, 0x1400,
            0x2800, 0x5000, 0xa000, 0x4000,
            0x20000, 0x50000, 0xa0000, 0x140000,
            0x280000, 0x500000, 0xa00000, 0x400000,
            0x2000000, 0x5000000, 0xa000000, 0x14000000,
            0x28000000, 0x50000000, 0xa0000000, 0x40000000,
            0x200000000, 0x500000000, 0xa00000000, 0x1400000000,
            0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000,
            0x20000000000, 0x50000000000, 0xa0000000000, 0x140000000000,
            0x280000000000, 0x500000000000, 0xa00000000000, 0x400000000000,
            0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000,
            0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000
        }
    };
    
    const U64 ep_target_lookup[64] = {
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        
        0x2000000, 0x5000000, 0xa000000, 0x14000000,
        0x28000000, 0x50000000, 0xa0000000, 0x40000000,
        0x200000000, 0x500000000, 0xa00000000, 0x1400000000,
        0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000,
        
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
    };
    
    const U64 occupy_square[64] = {
        0x1, 0x2, 0x4, 0x8,
        0x10, 0x20, 0x40, 0x80,
        0x100, 0x200, 0x400, 0x800,
        0x1000, 0x2000, 0x4000, 0x8000,
        0x10000, 0x20000, 0x40000, 0x80000,
        0x100000, 0x200000, 0x400000, 0x800000,
        0x1000000, 0x2000000, 0x4000000, 0x8000000,
        0x10000000, 0x20000000, 0x40000000, 0x80000000,
        0x100000000, 0x200000000, 0x400000000, 0x800000000,
        0x1000000000, 0x2000000000, 0x4000000000, 0x8000000000,
        0x10000000000, 0x20000000000, 0x40000000000, 0x80000000000,
        0x100000000000, 0x200000000000, 0x400000000000, 0x800000000000,
        0x1000000000000, 0x2000000000000, 0x4000000000000, 0x8000000000000,
        0x10000000000000, 0x20000000000000, 0x40000000000000, 0x80000000000000,
        0x100000000000000, 0x200000000000000, 0x400000000000000, 0x800000000000000,
        0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000
    };
    const int index_debruges64[64] = {
        0, 47,  1, 56, 48, 27,  2, 60,
       57, 49, 41, 37, 28, 16,  3, 61,
       54, 58, 35, 52, 50, 42, 21, 44,
       38, 32, 29, 23, 17, 11,  4, 62,
       46, 55, 26, 59, 40, 36, 15, 53,
       34, 51, 20, 43, 31, 22, 10, 45,
       25, 39, 14, 33, 19, 30,  9, 24,
       13, 18,  8, 12,  7,  6,  5, 63
    };
    const U64 debruges = 0x03f79d71b4cb0a89;
};
#endif

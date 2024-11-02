//
//  bitboard_gen.cpp
//  Bitboard_Movegen
//
//  Created by Harry Chiu on 10/17/24.
//
#include "bitboard_gen.h"

inline U64 reverse(U64 b) {
    return OSSwapInt64(b);
}

U64 mirror(U64 x) {
   U64 k1 = (0x5555555555555555);
   U64 k2 = (0x3333333333333333);
   U64 k4 = (0x0f0f0f0f0f0f0f0f);
   x = ((x >> 1) & k1) +  2*(x & k1);
   x = ((x >> 2) & k2) +  4*(x & k2);
   x = ((x >> 4) & k4) + 16*(x & k4);
   return x;
}

std::string Square [64] {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
};

void print_move(uint16_t move){
    int source = (move >> 10) & 0x3f;
    int dest = (move >> 4) & 0x3f;
    int flag = move & 0x0f;
    std::cout<< Square[source] << Square[dest] << flag;
}


Bitboard_Gen::Bitboard_Gen(std::string fen){
    init_zobrist_keys();
    set_board(fen);
}

void Bitboard_Gen::set_board(std::string fen){
    
    int rank = 7;
    int file = 0;
    
    //wipe the board and fill it with 0s
    clear_board();
    zobrist_hash = 0;
    ply = 0;
    uint8_t castling_rights = 0;
    
    for(char& c : fen){
        if(c == '/'){
            rank--;
            file = 0;
        }else if(int (c) > int ('0') && int (c) < int('9')) { // c is between 1 and 8
            file += int (c) - int('0');
        }else if(!((rank * 16 + file) & 0x88)){
            int piece_type = fen_char_to_piece_type[(char) std::tolower(c)];
            int piece_color = isupper(c) ? WHITE : BLACK;
            add_piece(piece_color, piece_type, rank * 8 + file);
            file++;
        }else if(rank == 0 && file == 8){
            if(c == 'K'){
                castling_rights |= WKS_CASTLING_RIGHTS;
            }else if(c == 'Q'){
                castling_rights |= WQS_CASTLING_RIGHTS;
            }else if(c == 'k'){
                castling_rights |= BKS_CASTLING_RIGHTS;
            }else if(c == 'q'){
                castling_rights |= BQS_CASTLING_RIGHTS;
            }else if(c == 'w'){
                current_side = WHITE;
            }else if(c == 'b'){
                current_side = BLACK;
            }
        }
    }

    game_history[ply] = game_state(castling_rights, 0, 0);
}

void Bitboard_Gen::init_zobrist_keys(){
    PRNG rng(1070372);
    
    for(int piece = 4; piece < 16; piece++){
        for(int square = 0; square < 64; square++){
            zobrist_keys.piecesquare[piece][square] = rng.rand64();
        }
    }for(int i = 0; i < 16; i++)
        zobrist_keys.castling[i] = rng.rand64();
    zobrist_keys.color = rng.rand64();
    for(int i = 0; i < 40; i++)
        zobrist_keys.ep_squares[i] = rng.rand64();
}

//generates all pseudolegal moves
int Bitboard_Gen::generate_moves(uint16_t * move_list){
    uint16_t * og_list = move_list;
    U64 occupied = bitboards[WHITE] | bitboards[BLACK];
    U64 empty = ~occupied;
    U64 enemy_attacked_squares = generate_attacked_squares(!current_side);
    
    U64 side_pawn_board = bitboards[current_side] & bitboards[PAWN_BOARD];
    
    //only initializing one destboard
    U64 board;
    
    //castles is also added here FYI
    //branching bad yada yada, but im lazy, if(BLACK), else->WHITE
    if(current_side){
        //get all possible pawn pushes except for ones that are promotions
        board = (side_pawn_board >> 8) & empty & (~rank_masks[0]);
        U64 double_pawn_pushes = (board >> 8) & empty & rank_masks[4];
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest + 8) << 10) | (dest << 4) | QUIET_FLAG;
        }
        while(double_pawn_pushes){
            int dest = pop_lsb(&double_pawn_pushes);
            *move_list++ = ((dest + 16) << 10) | (dest << 4) | DOUBLE_PAWN_PUSH_FLAG;
        }
        //get all pawn captures except promos
        board = (side_pawn_board >> 9) & bitboards[WHITE] & (~file_masks[7]) & (~rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest + 9) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        board = (side_pawn_board >> 7) & bitboards[WHITE] & (~file_masks[0]) & (~rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest + 7) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        
        //PROMOTIONS::
        board = (side_pawn_board >> 8) & empty & (rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            move_list = add_promo_moves(dest + 8, dest, move_list);
        }
        board = (side_pawn_board >> 9) & bitboards[WHITE] & (~file_masks[7]) & (rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            move_list = add_promo_cap_moves(dest + 9, dest, move_list);
        }
        board = (side_pawn_board >> 7) & bitboards[WHITE] & (~file_masks[0]) & (rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            move_list = add_promo_cap_moves(dest + 7, dest, move_list);
        }
        
        //EN PASSANT
        U64 possible_passants = ep_target_lookup[game_history[ply].ep_target] & side_pawn_board;
        while(possible_passants){
            int source = pop_lsb(&possible_passants);
            *move_list++ = (source << 10) | ((game_history[ply].ep_target - 8) << 4) |EN_PASSANT_FLAG;
        }
        
        //black ks castle logic, first line: if h8 and e8 has been undisturbed
        //2nd line: if not in check and not castling through check
        //3d line: squares between are empty
        if((game_history[ply].castling_rights & BKS_CASTLING_RIGHTS)
           && !(enemy_attacked_squares & (occupy_square[60] | occupy_square[61] | occupy_square[62]))
           && !(occupied & (occupy_square[61] | occupy_square[62]))){
            *move_list++ = (60 << 10) | (62 << 4) |KINGSIDE_CASTLE_FLAG;
        }
        //black qs castle
        if((game_history[ply].castling_rights & BQS_CASTLING_RIGHTS)
           && !(enemy_attacked_squares & (occupy_square[60] | occupy_square[59] | occupy_square[58]))
           && !(occupied & (occupy_square[57] | occupy_square[58] | occupy_square[59]))){
            *move_list++ = (60 << 10) | (58 << 4) |QUEENSIDE_CASTLE_FLAG;
        }
    }
    else{
        U64 board = (side_pawn_board << 8) & empty & (~rank_masks[7]);
        U64 double_pawn_pushes = (board << 8) & empty & rank_masks[3];
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest - 8) << 10) | (dest << 4) | QUIET_FLAG;
        }while(double_pawn_pushes){
            int dest = pop_lsb(&double_pawn_pushes);
            *move_list++ = ((dest - 16) << 10) | (dest << 4) | DOUBLE_PAWN_PUSH_FLAG;
        }
        
        board = (side_pawn_board << 9) & bitboards[BLACK] & (~file_masks[0]) & (~rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest - 9) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        board = (side_pawn_board << 7) & bitboards[BLACK] & (~file_masks[7]) & (~rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest - 7) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        
        //PROMOTIONS:
        board = (side_pawn_board << 8) & empty & (rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            move_list = add_promo_moves(dest - 8, dest, move_list);
        }
        board = (side_pawn_board << 9) & bitboards[BLACK] & (~file_masks[0]) & (rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            move_list = add_promo_cap_moves(dest - 9, dest, move_list);
        }
        board = (side_pawn_board << 7) & bitboards[BLACK] & (~file_masks[7]) & (rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            move_list = add_promo_cap_moves(dest - 7, dest, move_list);
        }
        
        
        U64 possible_passants = ep_target_lookup[game_history[ply].ep_target] & side_pawn_board;
        while(possible_passants){
            int source = pop_lsb(&possible_passants);
            *move_list++ = (source << 10) | ((game_history[ply].ep_target + 8) << 4) | EN_PASSANT_FLAG;
        }

        //white castle logic, if confused, read the black castle comments above
        if((game_history[ply].castling_rights & WKS_CASTLING_RIGHTS)
           && !(enemy_attacked_squares & (occupy_square[4] | occupy_square[5] | occupy_square[6]))
           && !(occupied & (occupy_square[5] | occupy_square[6]))){
            *move_list++ = (4 << 10) | (6 << 4) | KINGSIDE_CASTLE_FLAG;
        }
        //white qs castle
        if((game_history[ply].castling_rights & WQS_CASTLING_RIGHTS)
           && !(enemy_attacked_squares & (occupy_square[2] | occupy_square[3] | occupy_square[4]))
           && !(occupied & (occupy_square[1] | occupy_square[2] | occupy_square[3]))){
            *move_list++ = (4 << 10) | (2 << 4) |QUEENSIDE_CASTLE_FLAG;
        }
    }
    
    board = bitboards[current_side] & bitboards[KNIGHT_BOARD];
    while(board){
        int source = pop_lsb(&board);
        U64 quiet_dests = knight_move_lookup[source] & empty;
        move_list = add_quiet_moves(source, quiet_dests, move_list);
        
        //intersect target moves with opposing pieces
        U64 capture_dests = knight_move_lookup[source] & bitboards[!current_side];
        move_list = add_capture_moves(source, capture_dests, move_list);
    }
    
    //generate all king moves, no need to loop since there is only one king
    //once attacked squares are calculated, king goes to only unattacked squares
    int king_source = get_square_index(bitboards[current_side] & bitboards[KING_BOARD]);
    U64 king_quiet_dests = king_move_lookup[king_source] & empty & (~enemy_attacked_squares);
    move_list = add_quiet_moves(king_source, king_quiet_dests, move_list);
    //intersect target moves with opposing pieces
    U64 king_capture_dests = king_move_lookup[king_source] & bitboards[!current_side]  & (~enemy_attacked_squares);
    move_list = add_capture_moves(king_source, king_capture_dests, move_list);
    
    //generate all bishop queen sliding moves
    board = bitboards[current_side] & (bitboards[BISHOP_BOARD] | bitboards[QUEEN_BOARD]);
    while(board){
        int source = pop_lsb(&board);
        U64 res = hyp_quint(source, occupied, diagonal_masks[source_to_diagonal[source]]);
        res |= hyp_quint(source, occupied, antidiagonal_masks[source_to_antidiagonal[source]]);
        move_list = add_quiet_moves(source, res & empty, move_list);
        move_list = add_capture_moves(source, res & bitboards[!current_side], move_list);
    }
    
    //generate all rook and queen moves
    board = bitboards[current_side] & (bitboards[ROOK_BOARD] | bitboards[QUEEN_BOARD]);
    while(board){
        int source = pop_lsb(&board);
        U64 res = hyp_quint(source, occupied, file_masks[source_to_file[source]]);
        res |= hyp_quint_horiz(source, occupied, rank_masks[source_to_rank[source]]);
        move_list = add_quiet_moves(source, res & empty, move_list);
        move_list = add_capture_moves(source, res & bitboards[!current_side], move_list);
    }
    
    return move_list - og_list;
}

//generates only capture moves, for the quiesence search
int Bitboard_Gen::generate_captures(uint16_t * move_list){
    uint16_t * og_list = move_list;
    U64 occupied = bitboards[WHITE] | bitboards[BLACK];
    U64 enemy_attacked_squares = generate_attacked_squares(!current_side);
    
    U64 side_pawn_board = bitboards[current_side] & bitboards[PAWN_BOARD];
    
    //only initializing one destboard
    U64 board;
    
    //Pawn captures
    if(current_side){
        //get all pawn captures except promos
        board = (side_pawn_board >> 9) & bitboards[WHITE] & (~file_masks[7]) & (~rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest + 9) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        board = (side_pawn_board >> 7) & bitboards[WHITE] & (~file_masks[0]) & (~rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest + 7) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        
        //PROMOTIONS::
        board = (side_pawn_board >> 9) & bitboards[WHITE] & (~file_masks[7]) & (rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            move_list = add_promo_cap_moves(dest + 9, dest, move_list);
        }
        board = (side_pawn_board >> 7) & bitboards[WHITE] & (~file_masks[0]) & (rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            move_list = add_promo_cap_moves(dest + 7, dest, move_list);
        }
    }
    else{
        board = (side_pawn_board << 9) & bitboards[BLACK] & (~file_masks[0]) & (~rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest - 9) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        board = (side_pawn_board << 7) & bitboards[BLACK] & (~file_masks[7]) & (~rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest - 7) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        
        //PROMOTIONS:
        board = (side_pawn_board << 9) & bitboards[BLACK] & (~file_masks[0]) & (rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            move_list = add_promo_cap_moves(dest - 9, dest, move_list);
        }
        board = (side_pawn_board << 7) & bitboards[BLACK] & (~file_masks[7]) & (rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            move_list = add_promo_cap_moves(dest - 7, dest, move_list);
        }
    }
    //knight captures
    board = bitboards[current_side] & bitboards[KNIGHT_BOARD];
    while(board){
        int source = pop_lsb(&board);
        //intersect target moves with opposing pieces
        U64 capture_dests = knight_move_lookup[source] & bitboards[!current_side];
        move_list = add_capture_moves(source, capture_dests, move_list);
    }
    
    //generate all king moves, no need to loop since there is only one king
    //once attacked squares are calculated, king goes to only unattacked squares
    int king_source = get_square_index(bitboards[current_side] & bitboards[KING_BOARD]);
    //intersect target moves with opposing pieces
    U64 king_capture_dests = king_move_lookup[king_source] & bitboards[!current_side]  & (~enemy_attacked_squares);
    move_list = add_capture_moves(king_source, king_capture_dests, move_list);
    
    //generate all bishop queen sliding moves
    board = bitboards[current_side] & (bitboards[BISHOP_BOARD] | bitboards[QUEEN_BOARD]);
    while(board){
        int source = pop_lsb(&board);
        U64 res = hyp_quint(source, occupied, diagonal_masks[source_to_diagonal[source]]);
        res |= hyp_quint(source, occupied, antidiagonal_masks[source_to_antidiagonal[source]]);
        move_list = add_capture_moves(source, res & bitboards[!current_side], move_list);
    }
    
    //generate all rook and queen moves
    board = bitboards[current_side] & (bitboards[ROOK_BOARD] | bitboards[QUEEN_BOARD]);
    while(board){
        int source = pop_lsb(&board);
        U64 res = hyp_quint(source, occupied, file_masks[source_to_file[source]]);
        res |= hyp_quint_horiz(source, occupied, rank_masks[source_to_rank[source]]);
        move_list = add_capture_moves(source, res & bitboards[!current_side], move_list);
    }
    return move_list - og_list;
}

bool Bitboard_Gen::is_in_check(){
    int king_source = get_square_index(bitboards[!current_side] & bitboards[KING_BOARD]);
    U64 occupied = bitboards[WHITE] | bitboards[BLACK];
    //potential squares the king can be attacked by pawns
    U64 board = pawn_capture_lookup[!current_side][king_source];
    if(board & bitboards[current_side] & bitboards[PAWN_BOARD])
        return true;
    
    //knights and kings are easy
    if(knight_move_lookup[king_source] & bitboards[current_side] & bitboards[KNIGHT_BOARD])
        return true;
    if(king_move_lookup[king_source]  & bitboards[current_side] & bitboards[KING_BOARD])
        return true;
    
    //check diagonal rays
    board = hyp_quint(king_source, occupied, diagonal_masks[source_to_diagonal[king_source]]);
    board |= hyp_quint(king_source, occupied, antidiagonal_masks[source_to_antidiagonal[king_source]]);
    if(board & bitboards[current_side] & (bitboards[BISHOP_BOARD] | bitboards[QUEEN_BOARD])){
        return true;
    }
    
    //check orthogonal rays
    board = hyp_quint(king_source, occupied, file_masks[source_to_file[king_source]]);
    board |= hyp_quint_horiz(king_source, occupied, rank_masks[source_to_rank[king_source]]);
    if(board & bitboards[current_side] & (bitboards[ROOK_BOARD] | bitboards[QUEEN_BOARD])){
        return true;
    }
    return false;
}

//generates all squares attacked by side
U64 Bitboard_Gen::generate_attacked_squares(bool side){
    //union of all attacked squares
    U64 attacked_squares = 0;
    //dont count other king in occupied, since sliding pieces x-ray the king
    U64 occupied = (bitboards[BLACK] | bitboards[WHITE]) & ~(bitboards[!side] & bitboards[KING_BOARD]);
    
    //bulk process all pawn attacks
    U64 side_pawn_board = bitboards[side] & bitboards[PAWN_BOARD];
    if(side){
        attacked_squares |= (side_pawn_board >> 9) & (~file_masks[7]);
        attacked_squares |= (side_pawn_board >> 7) & (~file_masks[0]);
    }else{
        attacked_squares |= (side_pawn_board << 9) & (~file_masks[0]);
        attacked_squares |= (side_pawn_board << 7) & (~file_masks[7]);
    }
    
    //get all knight moves
    U64 side_knight_board = bitboards[side] & bitboards[KNIGHT_BOARD];
    while(side_knight_board){
        int source = pop_lsb(&side_knight_board);
        attacked_squares |= knight_move_lookup[source];
    }
    
    //get the king moves, errors if no king
    int king_source = get_square_index(bitboards[side] & bitboards[KING_BOARD]);
    attacked_squares |= king_move_lookup[king_source];

    //get all diag sliders
    U64 side_diagonal_board = bitboards[side] & (bitboards[BISHOP_BOARD] | bitboards[QUEEN_BOARD]);
    while(side_diagonal_board){
        int source = pop_lsb(&side_diagonal_board);
        attacked_squares |= hyp_quint(source, occupied, diagonal_masks[source_to_diagonal[source]]);
        attacked_squares |= hyp_quint(source, occupied, antidiagonal_masks[source_to_antidiagonal[source]]);
    }
    
    U64 side_orthog_board = bitboards[side] & (bitboards[ROOK_BOARD] | bitboards[QUEEN_BOARD]);
    while(side_orthog_board){
        int source = pop_lsb(&side_orthog_board);
        attacked_squares |= hyp_quint(source, occupied, file_masks[source_to_file[source]]);
        attacked_squares |= hyp_quint_horiz(source, occupied, rank_masks[source_to_rank[source]]);
    }
    
    return attacked_squares;
}

void Bitboard_Gen::make_move(uint16_t move){
    uint8_t new_castling_rights = game_history[ply].castling_rights;
    zobrist_hash ^= zobrist_keys.castling[new_castling_rights];
    zobrist_hash ^= zobrist_keys.ep_squares[game_history[ply].ep_target];
    int ep_target = 0;
    int captured_piece = 0;
    
    int source = (move >> 10) & 0x3f;
    int dest = (move >> 4) & 0x3f;
    int flag = move & 0x0f;
    
    //IMPLEMENT THE FUCKIGN CASTLING LOGIC HEEEEREEE RAAAAAHHHHHHHHHHH
    switch (source){
        case 0: //a1
            new_castling_rights &= ~WQS_CASTLING_RIGHTS;
            break;
        case 4: //e1
            new_castling_rights &= ~(WQS_CASTLING_RIGHTS | WKS_CASTLING_RIGHTS);
            break;
        case 7: //h1
            new_castling_rights &= ~WKS_CASTLING_RIGHTS;
            break;
        case 56: //a8
            new_castling_rights &= ~BQS_CASTLING_RIGHTS;
            break;
        case 60: //e8
            new_castling_rights &= ~(BQS_CASTLING_RIGHTS | BKS_CASTLING_RIGHTS);
            break;
        case 63: //h8
            new_castling_rights &= ~BKS_CASTLING_RIGHTS;
            break;
    }
    switch (dest){
        case 0: //a1
            new_castling_rights &= ~WQS_CASTLING_RIGHTS;
            break;
        case 4: //e1
            new_castling_rights &= ~(WQS_CASTLING_RIGHTS | WKS_CASTLING_RIGHTS);
            break;
        case 7: //h1
            new_castling_rights &= ~WKS_CASTLING_RIGHTS;
            break;
        case 56: //a8
            new_castling_rights &= ~BQS_CASTLING_RIGHTS;
            break;
        case 60: //e8
            new_castling_rights &= ~(BQS_CASTLING_RIGHTS | BKS_CASTLING_RIGHTS);
            break;
        case 63: //h8
            new_castling_rights &= ~BKS_CASTLING_RIGHTS;
            break;
    }

    if(flag == QUIET_FLAG){
        move_piece(source, dest);
    }
    else if(flag == DOUBLE_PAWN_PUSH_FLAG){
        move_piece(source, dest);
        ep_target = dest;
    }
    else if(flag == CAPTURE_FLAG){
        captured_piece = mailbox[dest];
        remove_piece(dest);
        move_piece(source, dest);
    }
    else if(flag == KINGSIDE_CASTLE_FLAG){
        move_piece(source, dest);
        move_piece(source + 3, source + 1);
    }
    else if(flag == QUEENSIDE_CASTLE_FLAG){
        move_piece(source, dest);
        move_piece(source - 4, source - 1);
    }
    else if(flag == EN_PASSANT_FLAG){
        captured_piece = mailbox[game_history[ply].ep_target];
        move_piece(source, dest);
        remove_piece(game_history[ply].ep_target);
    }
    else if(flag == BISHOP_PROMO_FLAG){
        remove_piece(source);
        add_piece(current_side, BISHOP_BOARD, dest);
    }
    else if(flag == ROOK_PROMO_FLAG){
        remove_piece(source);
        add_piece(current_side, ROOK_BOARD, dest);
    }
    else if(flag == KNIGHT_PROMO_FLAG){
        remove_piece(source);
        add_piece(current_side, KNIGHT_BOARD, dest);
    }
    else if(flag == QUEEN_PROMO_FLAG){
        remove_piece(source);
        add_piece(current_side, QUEEN_BOARD, dest);
    }
    else if(flag == BISHOP_PROMO_CAP_FLAG){
        captured_piece = mailbox[dest];
        remove_piece(source);
        remove_piece(dest);
        add_piece(current_side, BISHOP_BOARD, dest);
    }
    else if(flag == ROOK_PROMO_CAP_FLAG){
        captured_piece = mailbox[dest];
        remove_piece(source);
        remove_piece(dest);
        add_piece(current_side, ROOK_BOARD, dest);
    }
    else if(flag == KNIGHT_PROMO_CAP_FLAG){
        captured_piece = mailbox[dest];
        remove_piece(source);
        remove_piece(dest);
        add_piece(current_side, KNIGHT_BOARD, dest);
    }
    else if(flag == QUEEN_PROMO_CAP_FLAG){
        captured_piece = mailbox[dest];
        remove_piece(source);
        remove_piece(dest);
        add_piece(current_side, QUEEN_BOARD, dest);
    }
    ply++;
    game_history[ply] = game_state(new_castling_rights, captured_piece, ep_target);
    current_side = !current_side;
    zobrist_hash ^= zobrist_keys.color;
    zobrist_hash ^= zobrist_keys.castling[new_castling_rights];
    zobrist_hash ^= zobrist_keys.ep_squares[game_history[ply].ep_target];
}

void Bitboard_Gen::unmake_move(uint16_t move){
    zobrist_hash ^= zobrist_keys.ep_squares[game_history[ply].ep_target];
    zobrist_hash ^= zobrist_keys.castling[game_history[ply].castling_rights];
    int source = (move >> 10) & 0x3f;
    int dest = (move >> 4) & 0x3f;
    int flag = move & 0x0f;
    
    if(flag == QUIET_FLAG | flag == DOUBLE_PAWN_PUSH_FLAG){
        add_piece(mailbox[dest], source);
        remove_piece(dest);
    }
    else if(flag == CAPTURE_FLAG){
        add_piece(mailbox[dest], source);
        remove_piece(dest);
        add_piece(game_history[ply].captured, dest);
    }
    else if(flag == KINGSIDE_CASTLE_FLAG){
        
        add_piece(mailbox[dest], source);
        remove_piece(dest);
        add_piece(mailbox[source + 1], source + 3);
        remove_piece(source + 1);
    }
    else if(flag == QUEENSIDE_CASTLE_FLAG){
        add_piece(mailbox[dest], source);
        remove_piece(dest);
        add_piece(mailbox[source - 1], source - 4);
        remove_piece(source -1);
    }
    else if(flag == EN_PASSANT_FLAG){
        move_piece(dest, source);
        add_piece(game_history[ply].captured, game_history[ply - 1].ep_target);
    }
    else if((flag & 12) == 8){
        remove_piece(dest);
        add_piece(!current_side, PAWN_BOARD, source);
    }
    else if((flag & 12) == 12){
        remove_piece(dest);
        add_piece(game_history[ply].captured, dest);
        add_piece(!current_side, PAWN_BOARD, source);
    }
    current_side = !current_side;
    zobrist_hash ^= zobrist_keys.color;
    ply--;
    zobrist_hash ^= zobrist_keys.castling[game_history[ply].castling_rights];
    zobrist_hash ^= zobrist_keys.ep_squares[game_history[ply].ep_target];
}

U64 Bitboard_Gen::perft(int depth){
    if (depth == 0){
        return 1ULL;
    }
    U64 nodes = 0;
    uint16_t move_list[256];
    int num_moves;
    num_moves = generate_moves(move_list);
    for(int i = 0; i < num_moves; i++){
        make_move(move_list[i]);
        if(!is_in_check()){
            nodes += perft(depth - 1);
        }
        unmake_move(move_list[i]);
    }
    return nodes;
}

//only works for vertical and diag
U64 Bitboard_Gen::hyp_quint(int source, U64 occupied, U64 mask) {
    return (((mask & occupied) - occupy_square[source] * 2) ^
            reverse(reverse(mask & occupied) - reverse(occupy_square[source]) * 2)) & mask;
}

//only works for horizontal
U64 Bitboard_Gen::hyp_quint_horiz(int source, U64 occupied, U64 mask) {
    return (((mask & occupied) - occupy_square[source] * 2) ^
            mirror(mirror(mask & occupied) - mirror(occupy_square[source]) * 2)) & mask;
}

inline uint16_t * Bitboard_Gen::add_quiet_moves(int source, U64 dests, uint16_t * move_list){
    while(dests){
        int dest = pop_lsb(&dests);
        *move_list++ = (source << 10) | (dest << 4) | QUIET_FLAG;
    }return move_list;
}

inline uint16_t * Bitboard_Gen::add_capture_moves(int source, U64 dests, uint16_t * move_list){
    while(dests){
        int dest = pop_lsb(&dests);
        *move_list++ = (source << 10) | (dest << 4) | CAPTURE_FLAG;
    }return move_list;
}

inline uint16_t * Bitboard_Gen::add_promo_moves(int source, int dest, uint16_t * move_list){
    *move_list++ = (source << 10) | (dest << 4) | KNIGHT_PROMO_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | BISHOP_PROMO_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | ROOK_PROMO_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | QUEEN_PROMO_FLAG;
    return move_list;
}

inline uint16_t * Bitboard_Gen::add_promo_cap_moves(int source, int dest, uint16_t * move_list){
    *move_list++ = (source << 10) | (dest << 4) | KNIGHT_PROMO_CAP_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | BISHOP_PROMO_CAP_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | ROOK_PROMO_CAP_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | QUEEN_PROMO_CAP_FLAG;
    return move_list;
}

void Bitboard_Gen::move_piece(int source, int dest){
    zobrist_hash ^= zobrist_keys.piecesquare[mailbox[source]][source] ^ zobrist_keys.piecesquare[mailbox[source]][dest];
    U64 mask = occupy_square[source] | occupy_square[dest];
    bitboards[mailbox[source] & 1] ^= mask;
    bitboards[mailbox[source] >> 1] ^= mask;
    mailbox[dest] = mailbox[source];
    mailbox[source] = 0;
}

void Bitboard_Gen::add_piece(int piece_color, int piece_type, int square_index){
    bitboards[piece_type] |= occupy_square[square_index];
    bitboards[piece_color] |= occupy_square[square_index];
    mailbox[square_index] = piece_color + (piece_type << 1);
    zobrist_hash ^= zobrist_keys.piecesquare[mailbox[square_index]][square_index];
}

void Bitboard_Gen::add_piece(int piece, int square_index){
    bitboards[piece & 1] |= occupy_square[square_index];
    bitboards[piece >> 1] |= occupy_square[square_index];
    mailbox[square_index] = piece;
    zobrist_hash ^= zobrist_keys.piecesquare[piece][square_index];
}

void Bitboard_Gen::remove_piece(int source){
    zobrist_hash ^= zobrist_keys.piecesquare[mailbox[source]][source];
    bitboards[mailbox[source] & 1] &= ~occupy_square[source];
    bitboards[mailbox[source] >> 1] &= ~occupy_square[source];
    mailbox[source] = 0;
    
}

void Bitboard_Gen::clear_board(){
    for(int i = 0; i < 64; i++)
        mailbox[i] = 0;
    for(int i = 0; i < 8; i++){
        bitboards[i] = 0;
    }
}

bool Bitboard_Gen::check_consistency(){
    U64 board;
    for(uint16_t i = PAWN_BOARD; i <= KING_BOARD; i++){
        for(uint16_t side = 0; side < 2; side++){
            int piece = side + (i << 1);
            board = bitboards[i] & bitboards[side];
            while(board){
                int source = pop_lsb(&board);
                if(mailbox[source] != piece)
                    return false;
            }
        }
    }
    return true;
}

char print_piece_arr[16] = {'0', '1', '2', '3', 'P', 'p', 'B', 'b', 'N', 'n', 'R', 'r', 'Q', 'q', 'K', 'k'};

void Bitboard_Gen::print_board(){
    for(int i = 7; i >= 0; i--){
        for(int j = 0; j < 8; j++){
            if(mailbox[i * 8 + j])
                std::cout << print_piece_arr[mailbox[i * 8 + j]];
            else
                std::cout << ".";
        }std::cout << std::endl;
    }
}

void Bitboard_Gen::print_bit_boards(){
    for(int i = 0; i < 8; i++){
        if(bitboards[i]){
            print_u64(bitboards[i]);
            std::cout << "\n\n";
        }
    }
}

void Bitboard_Gen::print_u64(U64 bitboard){
    for(int i = 7; i >= 0; i--){
        for(int j = 0; j < 8; j++){
            U64 index = 8 * i + j;
            U64 res = 1;
            res = res << index;
            if(res & bitboard){
                std::cout << 1;
            }else{
                std::cout << 0;
            }std::cout << " ";
        }std::cout << '\n';
    }
}

Bitboard_Gen::Bitboard_Gen(){
    clear_board();
}

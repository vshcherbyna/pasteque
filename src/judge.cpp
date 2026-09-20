/*
*  pastèque - uci chess engine
*
*  Copyright (C) 2018 by Volodymyr M. Shcherbyna <volodymyr@shcherbyna.com>
*
*      This file is part of pastèque.
*
*  pastèque is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  pastèque is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with pastèque.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "judge.h"

pasteque_namespace_begin

Taper PIECE_SQUARE[16][64];

static const Taper PIECE_VALUES[8] = {
    Taper(   0,    0),   // EMPTY
    Taper( -93, -177),   // KNIGHT
    Taper(  28,   53),   // PAWN
    Taper(-141,  -78),   // KING
    Taper(   0,    0),   // unused
    Taper(-141,  -56),   // BISHOP
    Taper( 199,   58),   // ROOK
    Taper( 118, -160)    // QUEEN
};

static const int KNIGHT_OPENING[64] = {
    -180,  -51, -200, -175, -110, -114,  215,  173,
    -198,  -41,  111,  -66, -120, -140, -149,  -31,
    -194,  110,   88,  -42,   44, -222,  247, -191,
     -54,  228, -193,  198,  230, -254,  254,   96,
      15,   93,  181,   18,  193,  -62, -243, -122,
     -93, -104, -141,  209, -146,  178, -199, -158,
     -11,  188,  -48,  -62, -248,   -1,   68, -138,
     250,  -97, -166,  -68,   76,   19, -178,  -74
};

static const int KNIGHT_CLOSING[64] = {
     241,  176,  217, -205,  202,   -5, -238,  196,
     215, -200, -156, -106,  -18, -118,  148,  -59,
     132, -256,  199,  -96, -103, -136,   22,   54,
    -101,  -28,  131, -188,  109,  127,  145,  243,
     -50,  147, -205,  -79,  108,   94,  139, -197,
    -211, -153, -130,   86,  -30,  -23,  172, -202,
    -233,  133,  -64,  187, -229, -165,  -19,  186,
     -31,  125,  202,  204, -190,  122,  134,  168
};

static const int PAWN_OPENING[64] = {
     180,  246,  145, -116,  139,   29,   45, -174,
    -165,  103,  194,   24,   69, -145,  102, -120,
    -105,  -57, -182,  -11,   74,   41, -113,   60,
      62,  182, -181,  211, -141, -240,   35,  236,
    -181,  245,  -86,    6,   18, -198, -122,  218,
      96,  104,   37,   89,    7, -122,   53,  208,
    -137,   78,  -70,  -40,   81, -195,   40,    0,
     129,  -19,   48,  -15,  164,  237,  -67,   76
};

static const int PAWN_CLOSING[64] = {
     155,   99, -107,  -41,  -92,   32,  -84,  235,
     231,   47,  137,  -48, -229, -223, -111,  133,
     -81,  -99,  175,  -75,  -29,   77, -119,   50,
      29, -230,  -72, -110,  -43, -124,  216, -252,
    -250,  147,  110,  158, -227, -239, -140,  175,
    -141,  159,  100, -234,  156, -234,   36, -169,
    -208,   53,  -11, -113, -222, -256,  148, -240,
    -158,  -73, -207,  -24,   42,   63,   11, -133
};

static const int KING_OPENING[64] = {
      80,  175,  118, -129,  189, -243, -224,  174,
     244,  208,  -14,   41, -244,  253,  160,  143,
     176,  -44,  232, -220,  216, -139,  136,  195,
    -116,  -15,   35,   11,  -51, -253,   87,   73,
    -176,   64, -189, -164,  180,  132,  140,  -65,
    -178,  -38, -228,  206,    1,   26,  162,  110,
     -47,  -55,  240,   29, -198, -161,   24,  180,
    -140,  -63, -149, -180,   45,  -97, -189,  151
};

static const int KING_CLOSING[64] = {
    -144,   92, -130,  212,  112,   67,  -20, -155,
    -204,  -87, -208,  128,  -32,  213,  -31, -245,
      28,  201, -200, -247,  205,   97,  249,  253,
    -175,  110,   -7,  246, -113, -168,   24,  248,
     -98,  132, -231,  122, -151,  185,  188, -152,
      39,  -93, -180, -213, -212,  -99, -206, -184,
      -8,  205,  211,   36,  126,  -25,   98,  -87,
     107,  141,   31,   16,   36,   44,   83,  -10
};

static const int BISHOP_OPENING[64] = {
     231,  137,  139,  -90,  -95,  219,   76, -195,
      21, -233, -249, -121,  -41, -130,  169, -163,
    -154,  112,  244, -220,  112,  -99,  213,  215,
    -205,  121,  148,   50, -174,  149, -207, -115,
     -49,   15,   77, -223,  178,   59,   96,  154,
     106,  -97,  116,  -40,   85,  215,  -78,  147,
    -126,   -3,  106,   17,  -32,  -67,  -13, -153,
    -158, -124,  -81,  178,  188,  -17, -190, -104
};

static const int BISHOP_CLOSING[64] = {
     198, -127,   94, -220,   80, -198,  234, -221,
     114, -119, -215, -167, -218, -145, -156,   88,
     229, -154,  242,  252,  -57,    5,  -14,  -90,
     196,  -17, -144,  233, -157,  -30,   92,  -84,
     -80,   -3, -101,  190,   -4, -204, -249,   55,
     -20,   -1,   54,   78, -177,  246,  156, -116,
     -91,   78, -161,   30, -138, -219,  227,  110,
    -243,  -61, -119, -206, -185,  157,   47, -196
};

static const int ROOK_OPENING[64] = {
    -181,   94, -244, -158, -166,  121,   70,   13,
      59, -245,  240,   74,   82,  223, -190,   46,
    -125,  -63,  -91,  -66, -122,  151,  180, -223,
     -17,  222,  203,  227,   42,  252, -146, -194,
     209,  237,  241,  -30,  178,  -87,  -42, -103,
    -175,   27,  -80,  -26,  169, -251,   19,  111,
    -157,   41, -227,  235,  -50,  128,  -13,  150,
     110,  -78,  -58, -129, -159,  -21,   17,  132
};

static const int ROOK_CLOSING[64] = {
     225,   52,  152,   27,   32,  115,  211,   59,
    -131, -237, -170, -154,  -29,   38,   62,  140,
      96,  204,  -53,  106,   47,  -27,  -48,  -60,
     104, -184, -202,  -45,  -31,  192, -159, -164,
     164, -111, -223,  -84,  212, -129,  -83,    3,
     -84,  232,  185,  -31,  -59,  -96, -186,  -92,
     -18, -144,   17, -169,  243,  104, -254, -119,
     251, -204,  -33,  120, -156,    6,  249,  109
};

static const int QUEEN_OPENING[64] = {
       4, -174,  175, -147,  163,  -43,  255, -184,
     129, -238, -134, -104,  121,  155,   13,  -93,
     -86, -126,    5,  171,    6, -207, -131,   48,
      52,  -86,  -43,  188,   29,   28,  -38,  237,
    -147,  -31,   -4, -139, -248, -103,  167,   63,
     254,  158,   27,   74, -242, -183,  -84,  140,
      51,   71,  225,  206,  103,  138,  116,  -62,
      20,  -98, -190,  190,  219,  144,   50,   65
};

static const int QUEEN_CLOSING[64] = {
     -70,  132,  155,  158,   21, -139,   52, -139,
     -97,  -42,  231,  -12,  148,  180,  216,  165,
     -60, -216, -180,   84, -125,   73,   76,  -73,
     198,  254,  197, -201,  247,    9, -103,  162,
      24, -100,  158, -156,   91, -250, -133, -193,
     206,   16,  -64,  145,  -53,  218,  228,   62,
    -253, -238,  -48,  164, -241, -141,  -18,   71,
     169,   79,  -13, -174,  102,  -40,  229,  178
};

static const int PHASE_WEIGHTS[8] = { 0, 7, 0, 0, 0, 0, 0, 4 };

static const int * const TABLE_OPENING[8] = { nullptr, KNIGHT_OPENING, PAWN_OPENING, KING_OPENING, nullptr, BISHOP_OPENING, ROOK_OPENING, QUEEN_OPENING };

static const int * const TABLE_CLOSING[8] = { nullptr, KNIGHT_CLOSING, PAWN_CLOSING, KING_CLOSING, nullptr, BISHOP_CLOSING, ROOK_CLOSING, QUEEN_CLOSING };

void Judge::init() {

    static auto initialised = false;

    if (initialised)
        return;

    initialised = true;

    for (auto piece = 0; piece < 16; ++piece)
        for (auto square = 0; square < 64; ++square)
            PIECE_SQUARE[piece][square] = Taper();

    for (auto row = 0; row < 8; ++row) {
        for (auto file = 0; file < 8; ++file) {

            auto entry = row * 8 + file;
            auto white = square_of(file, 7 - row);
            auto black = square_of(file, row);

            for (int type = KNIGHT; type <= QUEEN; ++type) {
                if (!TABLE_OPENING[type])
                    continue;

                Taper score(PIECE_VALUES[type].opening + TABLE_OPENING[type][entry],
                           PIECE_VALUES[type].closing + TABLE_CLOSING[type][entry]);

                PIECE_SQUARE[piece_of(type, WHITE)][white] = score;
                PIECE_SQUARE[piece_of(type, BLACK)][black] = score;
            }
        }
    }
}

int Judge::phase(const Board & board) {
    auto phase = 0;

    for (int type = KNIGHT; type <= QUEEN; ++type)
        phase += PHASE_WEIGHTS[type] * (popCount(board.getPieces(piece_of(type, WHITE))) + popCount(board.getPieces(piece_of(type, BLACK))));

    return (phase > PHASE_MAX) ? PHASE_MAX : phase;
}

Taper Judge::pieceSquare(const Board & board) {

    Taper score;
    auto pieces = board.getAllPieces();

    while (pieces) {
        auto square = popFirstOne(pieces);
        auto piece  = board.getPiece(square);

        if (piece_color(piece) == WHITE)
            score += PIECE_SQUARE[piece][square];
        else
            score -= PIECE_SQUARE[piece][square];
    }

    return score;
}

int Judge::scale(const Board & board, int eval) {

    auto mating = board.getPieces(WHITE_PAWN)  | board.getPieces(BLACK_PAWN) | board.getPieces(WHITE_ROOK)  | board.getPieces(BLACK_ROOK) | board.getPieces(WHITE_QUEEN) | board.getPieces(BLACK_QUEEN);

    if (mating)
        return eval;

    auto minors = popCount(board.getPieces(WHITE_KNIGHT) | board.getPieces(WHITE_BISHOP) | board.getPieces(BLACK_KNIGHT) | board.getPieces(BLACK_BISHOP));

    return (minors <= 1) ? EVEN_SCORE : eval;
}

int Judge::evaluate(const Board & board) {

    auto score = pieceSquare(board);
    auto left  = phase(board);

    auto eval = (score.opening * left + score.closing * (PHASE_MAX - left)) / PHASE_MAX;

    if (board.getSide() == BLACK)
        eval = -eval;

    eval += TEMPO;

    return scale(board, eval);
}

pasteque_namespace_end

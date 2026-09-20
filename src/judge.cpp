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
    Taper( 320,  320),   // KNIGHT
    Taper( 100,  120),   // PAWN
    Taper(   0,    0),   // KING
    Taper(   0,    0),   // unused
    Taper( 330,  340),   // BISHOP
    Taper( 500,  540),   // ROOK
    Taper( 950,  960)    // QUEEN
};

static const int PAWN_OPENING[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
    50,  50,  50,  50,  50,  50,  50,  50,
    10,  10,  20,  30,  30,  20,  10,  10,
     5,   5,  10,  25,  25,  10,   5,   5,
     0,   0,   0,  20,  20,   0,   0,   0,
     5,  -5, -10,   0,   0, -10,  -5,   5,
     5,  10,  10, -20, -20,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0
};

static const int PAWN_CLOSING[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
    80,  80,  80,  80,  80,  80,  80,  80,
    50,  50,  50,  50,  50,  50,  50,  50,
    30,  30,  30,  30,  30,  30,  30,  30,
    15,  15,  15,  15,  15,  15,  15,  15,
     5,   5,   5,   5,   5,   5,   5,   5,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
};

static const int KNIGHT_ANY[64] = {
   -50, -40, -30, -30, -30, -30, -40, -50,
   -40, -20,   0,   0,   0,   0, -20, -40,
   -30,   0,  10,  15,  15,  10,   0, -30,
   -30,   5,  15,  20,  20,  15,   5, -30,
   -30,   0,  15,  20,  20,  15,   0, -30,
   -30,   5,  10,  15,  15,  10,   5, -30,
   -40, -20,   0,   5,   5,   0, -20, -40,
   -50, -40, -30, -30, -30, -30, -40, -50
};

static const int BISHOP_ANY[64] = {
   -20, -10, -10, -10, -10, -10, -10, -20,
   -10,   0,   0,   0,   0,   0,   0, -10,
   -10,   0,   5,  10,  10,   5,   0, -10,
   -10,   5,   5,  10,  10,   5,   5, -10,
   -10,   0,  10,  10,  10,  10,   0, -10,
   -10,  10,  10,  10,  10,  10,  10, -10,
   -10,   5,   0,   0,   0,   0,   5, -10,
   -20, -10, -10, -10, -10, -10, -10, -20
};

static const int ROOK_ANY[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
     5,  10,  10,  10,  10,  10,  10,   5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
     0,   0,   0,   5,   5,   0,   0,   0
};

static const int QUEEN_ANY[64] = {
   -20, -10, -10,  -5,  -5, -10, -10, -20,
   -10,   0,   0,   0,   0,   0,   0, -10,
   -10,   0,   5,   5,   5,   5,   0, -10,
    -5,   0,   5,   5,   5,   5,   0,  -5,
     0,   0,   5,   5,   5,   5,   0,  -5,
   -10,   5,   5,   5,   5,   5,   0, -10,
   -10,   0,   5,   0,   0,   0,   0, -10,
   -20, -10, -10,  -5,  -5, -10, -10, -20
};

static const int KING_OPENING[64] = {
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -20, -30, -30, -40, -40, -30, -30, -20,
   -10, -20, -20, -20, -20, -20, -20, -10,
    20,  20,   0,   0,   0,   0,  20,  20,
    20,  30,  10,   0,   0,  10,  30,  20
};

static const int KING_CLOSING[64] = {
   -50, -40, -30, -20, -20, -30, -40, -50,
   -30, -20, -10,   0,   0, -10, -20, -30,
   -30, -10,  20,  30,  30,  20, -10, -30,
   -30, -10,  30,  40,  40,  30, -10, -30,
   -30, -10,  30,  40,  40,  30, -10, -30,
   -30, -10,  20,  30,  30,  20, -10, -30,
   -30, -30,   0,   0,   0,   0, -30, -30,
   -50, -30, -30, -30, -30, -30, -30, -50
};

static const int PHASE_WEIGHTS[8] = { 0, 1, 0, 0, 0, 1, 2, 4 };

static const int * const TABLE_OPENING[8] = { nullptr, KNIGHT_ANY, PAWN_OPENING, KING_OPENING, nullptr, BISHOP_ANY, ROOK_ANY, QUEEN_ANY };

static const int * const TABLE_CLOSING[8] = { nullptr, KNIGHT_ANY, PAWN_CLOSING, KING_CLOSING, nullptr, BISHOP_ANY, ROOK_ANY, QUEEN_ANY };

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

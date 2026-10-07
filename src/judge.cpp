/*
*  pastèque - uci chess engine
*
*  Copyright (C) 2018-2026 Volodymyr Shcherbyna <volodymyr@shcherbyna.com>
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
    Taper( 190,  190),   // KNIGHT
    Taper( 100,  100),   // PAWN
    Taper(   0,    0),   // KING
    Taper(   0,    0),   // unused
    Taper( 227,  227),   // BISHOP
    Taper( 344,  344),   // ROOK
    Taper( 694,  694)    // QUEEN
};

static const int KNIGHT_OPENING[64] = {
     -29,  -28,  -19,  -27,  -28,  -18,  -18,  -21,
     -13,   -9,   -6,   -6,   -5,   20,    3,   -4,
      -2,   19,   14,   30,   37,   29,   27,   20,
      -6,   27,   27,   45,   47,   26,   35,    8,
      -3,    4,   28,   27,   27,   22,   18,   13,
     -18,   -9,   -2,    8,    6,    5,   -7,   -4,
     -21,  -11,   -7,  -24,  -17,   17,   -7,  -15,
     -42,  -31,  -24,  -19,  -18,  -22,  -19,  -32
};

static const int KNIGHT_CLOSING[64] = {
     -29,  -28,  -19,  -27,  -28,  -18,  -18,  -21,
     -13,   -9,   -6,   -6,   -5,   20,    3,   -4,
      -2,   19,   14,   30,   37,   29,   27,   20,
      -6,   27,   27,   45,   47,   26,   35,    8,
      -3,    4,   28,   27,   27,   22,   18,   13,
     -18,   -9,   -2,    8,    6,    5,   -7,   -4,
     -21,  -11,   -7,  -24,  -17,   17,   -7,  -15,
     -42,  -31,  -24,  -19,  -18,  -22,  -19,  -32
};

static const int PAWN_OPENING[64] = {
       0,    0,    0,    0,    0,    0,    0,    0,
      74,   77,   46,   44,   48,   52,   67,   64,
      41,   52,   29,   31,   35,   30,   40,   26,
      -4,   -8,   -4,   -1,   -7,   -7,   -6,   -9,
     -21,  -35,  -32,  -26,  -26,  -26,  -31,  -29,
     -38,  -43,  -37,  -40,  -34,  -34,  -22,  -37,
     -32,  -42,  -40,  -52,  -42,  -20,  -33,  -41,
       0,    0,    0,    0,    0,    0,    0,    0
};

static const int PAWN_CLOSING[64] = {
       0,    0,    0,    0,    0,    0,    0,    0,
      74,   77,   46,   44,   48,   52,   67,   64,
      41,   52,   29,   31,   35,   30,   40,   26,
      -4,   -8,   -4,   -1,   -7,   -7,   -6,   -9,
     -21,  -35,  -32,  -26,  -26,  -26,  -31,  -29,
     -38,  -43,  -37,  -40,  -34,  -34,  -22,  -37,
     -32,  -42,  -40,  -52,  -42,  -20,  -33,  -41,
       0,    0,    0,    0,    0,    0,    0,    0
};

static const int KING_OPENING[64] = {
      75,   62,   25,   23,   19,   19,   27,   32,
      17,   40,   20,   14,   18,   16,   16,    8,
      -7,    9,   18,   14,   16,   14,    1,   -5,
     -24,   -5,   -3,   -9,   -5,   -7,   -7,  -22,
     -36,  -23,  -24,  -17,  -19,  -24,  -18,  -23,
     -31,  -23,  -10,  -28,  -23,  -11,   -7,   -7,
     -32,   -2,    2,  -14,  -18,   12,   -2,  -10,
       2,   -4,  -16,  -11,    6,    4,   14,  -14
};

static const int KING_CLOSING[64] = {
      75,   62,   25,   23,   19,   19,   27,   32,
      17,   40,   20,   14,   18,   16,   16,    8,
      -7,    9,   18,   14,   16,   14,    1,   -5,
     -24,   -5,   -3,   -9,   -5,   -7,   -7,  -22,
     -36,  -23,  -24,  -17,  -19,  -24,  -18,  -23,
     -31,  -23,  -10,  -28,  -23,  -11,   -7,   -7,
     -32,   -2,    2,  -14,  -18,   12,   -2,  -10,
       2,   -4,  -16,  -11,    6,    4,   14,  -14
};

static const int BISHOP_OPENING[64] = {
     -13,   -9,   -8,  -13,  -13,  -16,  -17,  -15,
     -11,  -11,   -2,   -8,   -2,   -7,    0,   -8,
      -4,   12,   15,   10,   15,   25,    8,    5,
      18,    6,   24,   19,   13,   23,   15,    8,
       4,    3,   29,   15,   21,   20,    3,   -6,
      -7,    7,    4,    4,   10,    4,    5,   -7,
     -17,   -4,   -5,    6,   -1,   -6,    6,  -28,
     -23,  -36,   -2,   -6,  -14,   -4,  -13,  -25
};

static const int BISHOP_CLOSING[64] = {
     -13,   -9,   -8,  -13,  -13,  -16,  -17,  -15,
     -11,  -11,   -2,   -8,   -2,   -7,    0,   -8,
      -4,   12,   15,   10,   15,   25,    8,    5,
      18,    6,   24,   19,   13,   23,   15,    8,
       4,    3,   29,   15,   21,   20,    3,   -6,
      -7,    7,    4,    4,   10,    4,    5,   -7,
     -17,   -4,   -5,    6,   -1,   -6,    6,  -28,
     -23,  -36,   -2,   -6,  -14,   -4,  -13,  -25
};

static const int ROOK_OPENING[64] = {
      20,   16,   12,    1,   -9,   -7,    7,   19,
      33,   33,   21,   12,    8,    8,   15,   23,
      22,   17,   25,   19,   15,   23,   24,   25,
      -5,    6,   10,   11,    4,    7,   -1,   -4,
     -25,  -18,   -2,   -5,    4,    0,  -15,  -21,
     -35,  -16,  -19,  -11,   -9,  -11,   -3,  -28,
     -16,  -18,  -14,   -6,   -2,    5,   -9,  -23,
     -36,  -16,   -9,   -1,    1,    1,  -21,  -32
};

static const int ROOK_CLOSING[64] = {
      20,   16,   12,    1,   -9,   -7,    7,   19,
      33,   33,   21,   12,    8,    8,   15,   23,
      22,   17,   25,   19,   15,   23,   24,   25,
      -5,    6,   10,   11,    4,    7,   -1,   -4,
     -25,  -18,   -2,   -5,    4,    0,  -15,  -21,
     -35,  -16,  -19,  -11,   -9,  -11,   -3,  -28,
     -16,  -18,  -14,   -6,   -2,    5,   -9,  -23,
     -36,  -16,   -9,   -1,    1,    1,  -21,  -32
};

static const int QUEEN_OPENING[64] = {
     -12,  -13,   -6,  -11,   -4,    3,    4,   -8,
      -2,    9,   17,   15,   15,   40,   35,    4,
     -12,   14,   15,   43,   35,   29,   32,   14,
     -14,  -28,    8,   22,    3,   20,   -4,   18,
     -28,  -17,   -5,   -8,   -4,   -2,   -1,  -21,
     -29,    1,   -7,   -2,    2,    3,   16,  -20,
     -26,  -10,    9,    6,    5,   -8,   -6,   -6,
     -24,  -28,   -4,    5,  -15,  -13,  -28,  -19
};

static const int QUEEN_CLOSING[64] = {
     -12,  -13,   -6,  -11,   -4,    3,    4,   -8,
      -2,    9,   17,   15,   15,   40,   35,    4,
     -12,   14,   15,   43,   35,   29,   32,   14,
     -14,  -28,    8,   22,    3,   20,   -4,   18,
     -28,  -17,   -5,   -8,   -4,   -2,   -1,  -21,
     -29,    1,   -7,   -2,    2,    3,   16,  -20,
     -26,  -10,    9,    6,    5,   -8,   -6,   -6,
     -24,  -28,   -4,    5,  -15,  -13,  -28,  -19
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

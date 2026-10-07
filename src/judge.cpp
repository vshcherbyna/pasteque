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
    Taper( 209,  209),   // KNIGHT
    Taper( 100,  100),   // PAWN
    Taper(   0,    0),   // KING
    Taper(   0,    0),   // unused
    Taper( 242,  242),   // BISHOP
    Taper( 350,  350),   // ROOK
    Taper( 660,  660)    // QUEEN
};

static const int KNIGHT_OPENING[64] = {
     -64,  -32,  -28,  -15,  -13,  -24,  -31,  -47,
     -37,   -9,    9,    6,    8,   22,   -6,  -11,
       0,   14,   37,   36,   39,   38,   34,   10,
       5,   14,   30,   36,   45,   30,   31,   25,
      -8,    5,   13,   19,   29,   27,   12,    1,
     -17,  -14,    3,   15,   15,   16,    8,   -3,
     -28,  -26,   -3,   -5,   -2,   21,   -8,  -14,
     -32,  -35,  -29,  -20,  -23,   -9,  -26,  -31
};

static const int KNIGHT_CLOSING[64] = {
     -64,  -32,  -28,  -15,  -13,  -24,  -31,  -47,
     -37,   -9,    9,    6,    8,   22,   -6,  -11,
       0,   14,   37,   36,   39,   38,   34,   10,
       5,   14,   30,   36,   45,   30,   31,   25,
      -8,    5,   13,   19,   29,   27,   12,    1,
     -17,  -14,    3,   15,   15,   16,    8,   -3,
     -28,  -26,   -3,   -5,   -2,   21,   -8,  -14,
     -32,  -35,  -29,  -20,  -23,   -9,  -26,  -31
};

static const int PAWN_OPENING[64] = {
       0,    0,    0,    0,    0,    0,    0,    0,
     106,  115,   79,   64,   62,   51,   59,   69,
      38,   44,   16,   27,   25,   19,   13,   16,
     -10,   -9,  -22,  -22,  -20,  -31,  -15,  -34,
     -21,  -23,  -31,  -22,  -31,  -39,  -39,  -39,
     -26,  -22,  -26,  -42,  -44,  -36,  -19,  -42,
     -30,  -27,  -42,  -60,  -68,  -29,  -31,  -47,
       0,    0,    0,    0,    0,    0,    0,    0
};

static const int PAWN_CLOSING[64] = {
       0,    0,    0,    0,    0,    0,    0,    0,
     106,  115,   79,   64,   62,   51,   59,   69,
      38,   44,   16,   27,   25,   19,   13,   16,
     -10,   -9,  -22,  -22,  -20,  -31,  -15,  -34,
     -21,  -23,  -31,  -22,  -31,  -39,  -39,  -39,
     -26,  -22,  -26,  -42,  -44,  -36,  -19,  -42,
     -30,  -27,  -42,  -60,  -68,  -29,  -31,  -47,
       0,    0,    0,    0,    0,    0,    0,    0
};

static const int KING_OPENING[64] = {
    -253,  -41,   75,  198,  138,   44,  -82, -255,
     -51,   13,   86,  177,  117,   50,  -20,  -78,
     -11,   15,   30,   61,   51,   35,    8,  -20,
      -6,    2,    4,   12,   13,    9,    4,   -8,
     -11,   -7,   -8,   -3,  -14,  -10,    0,  -10,
       2,   -9,  -14,  -26,  -28,  -17,   -1,    0,
      -2,   -5,   -2,  -26,  -31,   -2,    4,   -1,
     -30,   -3,   -5,  -27,   -7,  -13,    8,  -20
};

static const int KING_CLOSING[64] = {
    -253,  -41,   75,  198,  138,   44,  -82, -255,
     -51,   13,   86,  177,  117,   50,  -20,  -78,
     -11,   15,   30,   61,   51,   35,    8,  -20,
      -6,    2,    4,   12,   13,    9,    4,   -8,
     -11,   -7,   -8,   -3,  -14,  -10,    0,  -10,
       2,   -9,  -14,  -26,  -28,  -17,   -1,    0,
      -2,   -5,   -2,  -26,  -31,   -2,    4,   -1,
     -30,   -3,   -5,  -27,   -7,  -13,    8,  -20
};

static const int BISHOP_OPENING[64] = {
     -30,  -13,  -10,  -11,  -12,  -21,  -15,  -20,
     -14,   -3,    3,   -2,    3,    3,    7,  -14,
       1,   17,    8,   31,   27,   24,   18,   16,
      -2,    4,   16,   22,   21,   11,   10,    7,
      -9,    6,   16,   19,   16,   14,    3,   -6,
     -10,   11,   10,    4,    6,    9,   -4,    2,
     -12,    1,    3,    3,   -1,   -6,   11,  -14,
     -20,  -22,  -13,  -16,  -18,  -24,  -26,  -19
};

static const int BISHOP_CLOSING[64] = {
     -30,  -13,  -10,  -11,  -12,  -21,  -15,  -20,
     -14,   -3,    3,   -2,    3,    3,    7,  -14,
       1,   17,    8,   31,   27,   24,   18,   16,
      -2,    4,   16,   22,   21,   11,   10,    7,
      -9,    6,   16,   19,   16,   14,    3,   -6,
     -10,   11,   10,    4,    6,    9,   -4,    2,
     -12,    1,    3,    3,   -1,   -6,   11,  -14,
     -20,  -22,  -13,  -16,  -18,  -24,  -26,  -19
};

static const int ROOK_OPENING[64] = {
      18,   24,   24,   25,   17,   20,   20,   21,
      18,   21,   25,   26,   34,   32,   23,   23,
      14,   22,   17,   25,   24,   25,   15,   18,
      -9,   -1,    6,   16,   11,    9,   -2,   -2,
      -9,   -7,   -6,    5,    3,   -1,   -4,  -20,
     -25,   -6,   -6,  -12,  -12,  -11,  -13,  -27,
     -53,  -20,  -15,  -15,  -12,   -4,  -30,  -52,
     -39,  -28,  -19,   -4,   -4,  -14,  -52,  -50
};

static const int ROOK_CLOSING[64] = {
      18,   24,   24,   25,   17,   20,   20,   21,
      18,   21,   25,   26,   34,   32,   23,   23,
      14,   22,   17,   25,   24,   25,   15,   18,
      -9,   -1,    6,   16,   11,    9,   -2,   -2,
      -9,   -7,   -6,    5,    3,   -1,   -4,  -20,
     -25,   -6,   -6,  -12,  -12,  -11,  -13,  -27,
     -53,  -20,  -15,  -15,  -12,   -4,  -30,  -52,
     -39,  -28,  -19,   -4,   -4,  -14,  -52,  -50
};

static const int QUEEN_OPENING[64] = {
     -36,   -8,    0,   17,   18,   11,    0,    0,
     -36,  -10,    2,   21,   35,   43,   35,   12,
     -26,   -4,   -4,   38,   52,   39,   44,   48,
     -17,  -17,  -13,   12,    9,   22,    8,   25,
     -11,  -13,  -11,    1,    3,   11,    6,   -5,
     -26,   -1,   -5,    7,   -6,   -2,   15,  -14,
     -38,  -19,   10,    0,   -1,   -8,  -11,  -31,
     -28,  -20,  -11,   -4,  -16,  -24,  -29,  -36
};

static const int QUEEN_CLOSING[64] = {
     -36,   -8,    0,   17,   18,   11,    0,    0,
     -36,  -10,    2,   21,   35,   43,   35,   12,
     -26,   -4,   -4,   38,   52,   39,   44,   48,
     -17,  -17,  -13,   12,    9,   22,    8,   25,
     -11,  -13,  -11,    1,    3,   11,    6,   -5,
     -26,   -1,   -5,    7,   -6,   -2,   15,  -14,
     -38,  -19,   10,    0,   -1,   -8,  -11,  -31,
     -28,  -20,  -11,   -4,  -16,  -24,  -29,  -36
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

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
    Taper( 199,  199),   // KNIGHT
    Taper( 100,  100),   // PAWN
    Taper(   0,    0),   // KING
    Taper(   0,    0),   // unused
    Taper( 226,  226),   // BISHOP
    Taper( 335,  335),   // ROOK
    Taper( 627,  627)    // QUEEN
};

static const int KNIGHT_OPENING[64] = {
     -68,  -31,  -12,   -4,   -4,  -13,  -28,  -49,
     -37,   -9,   13,   15,    6,   22,   -9,  -19,
     -12,   11,   23,   44,   41,   28,   23,   -1,
      -4,    1,   25,   42,   40,   38,   29,   20,
     -20,   -4,   14,   16,   31,   30,   11,    7,
     -23,   -5,    3,    6,   16,   15,   19,   -7,
     -25,  -17,   -4,   -6,   -1,   10,   -1,   -8,
     -30,  -33,  -24,  -21,  -16,   -6,  -22,  -23
};

static const int KNIGHT_CLOSING[64] = {
     -68,  -31,  -12,   -4,   -4,  -13,  -28,  -49,
     -37,   -9,   13,   15,    6,   22,   -9,  -19,
     -12,   11,   23,   44,   41,   28,   23,   -1,
      -4,    1,   25,   42,   40,   38,   29,   20,
     -20,   -4,   14,   16,   31,   30,   11,    7,
     -23,   -5,    3,    6,   16,   15,   19,   -7,
     -25,  -17,   -4,   -6,   -1,   10,   -1,   -8,
     -30,  -33,  -24,  -21,  -16,   -6,  -22,  -23
};

static const int PAWN_OPENING[64] = {
       0,    0,    0,    0,    0,    0,    0,    0,
      94,   96,   94,   85,   70,   65,   63,   54,
      36,   42,   31,   38,   32,   26,   19,   13,
     -11,  -18,  -21,  -20,  -13,  -29,  -28,  -35,
     -22,  -30,  -30,  -23,  -24,  -37,  -43,  -44,
     -26,  -30,  -35,  -46,  -49,  -43,  -15,  -42,
     -37,  -31,  -49,  -70,  -60,  -29,  -20,  -50,
       0,    0,    0,    0,    0,    0,    0,    0
};

static const int PAWN_CLOSING[64] = {
       0,    0,    0,    0,    0,    0,    0,    0,
      94,   96,   94,   85,   70,   65,   63,   54,
      36,   42,   31,   38,   32,   26,   19,   13,
     -11,  -18,  -21,  -20,  -13,  -29,  -28,  -35,
     -22,  -30,  -30,  -23,  -24,  -37,  -43,  -44,
     -26,  -30,  -35,  -46,  -49,  -43,  -15,  -42,
     -37,  -31,  -49,  -70,  -60,  -29,  -20,  -50,
       0,    0,    0,    0,    0,    0,    0,    0
};

static const int KING_OPENING[64] = {
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0
};

static const int KING_CLOSING[64] = {
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0
};

static const int BISHOP_OPENING[64] = {
     -32,  -15,  -13,  -10,  -12,  -21,  -16,  -20,
     -15,   -2,    3,    2,    3,    0,   -2,  -11,
     -11,   15,   13,   21,   13,   35,   17,   24,
      -2,   -1,   17,   24,   27,   16,   19,    4,
      -9,    2,   12,   28,   16,   16,    1,    1,
      -5,    3,   10,   -2,    2,    5,   12,   -2,
      -9,   -3,   -2,   -7,   -1,   -1,    9,   -7,
     -17,  -18,  -17,  -17,  -18,  -23,  -15,  -16
};

static const int BISHOP_CLOSING[64] = {
     -32,  -15,  -13,  -10,  -12,  -21,  -16,  -20,
     -15,   -2,    3,    2,    3,    0,   -2,  -11,
     -11,   15,   13,   21,   13,   35,   17,   24,
      -2,   -1,   17,   24,   27,   16,   19,    4,
      -9,    2,   12,   28,   16,   16,    1,    1,
      -5,    3,   10,   -2,    2,    5,   12,   -2,
      -9,   -3,   -2,   -7,   -1,   -1,    9,   -7,
     -17,  -18,  -17,  -17,  -18,  -23,  -15,  -16
};

static const int ROOK_OPENING[64] = {
      25,   26,   22,   23,   16,   16,   16,   20,
      30,   36,   35,   31,   27,   31,   24,   30,
      16,   24,   24,   27,   26,   26,   21,   18,
      -2,   -3,    8,   12,    9,   10,    2,    1,
     -19,  -18,   -5,   -1,    0,   -2,  -10,  -12,
     -35,  -21,  -13,  -16,  -12,  -10,   -9,  -39,
     -46,  -22,  -14,  -14,  -10,   -4,  -25,  -73,
     -39,  -28,  -22,   -9,   -5,   -8,  -45,  -41
};

static const int ROOK_CLOSING[64] = {
      25,   26,   22,   23,   16,   16,   16,   20,
      30,   36,   35,   31,   27,   31,   24,   30,
      16,   24,   24,   27,   26,   26,   21,   18,
      -2,   -3,    8,   12,    9,   10,    2,    1,
     -19,  -18,   -5,   -1,    0,   -2,  -10,  -12,
     -35,  -21,  -13,  -16,  -12,  -10,   -9,  -39,
     -46,  -22,  -14,  -14,  -10,   -4,  -25,  -73,
     -39,  -28,  -22,   -9,   -5,   -8,  -45,  -41
};

static const int QUEEN_OPENING[64] = {
     -35,  -13,   10,   23,   29,   23,    2,   -5,
     -34,  -15,    9,   23,   38,   46,   26,   12,
     -15,   -5,   11,   40,   52,   58,   40,   43,
     -23,  -18,    1,   14,   14,   26,    9,   24,
     -25,  -17,   -5,    0,    4,   11,    0,   -1,
     -25,  -11,  -41,   -5,  -10,  -14,    7,  -11,
     -23,  -19,   -6,   -6,   -4,   -9,  -17,  -20,
     -13,  -22,  -20,  -10,  -17,  -25,  -29,  -24
};

static const int QUEEN_CLOSING[64] = {
     -35,  -13,   10,   23,   29,   23,    2,   -5,
     -34,  -15,    9,   23,   38,   46,   26,   12,
     -15,   -5,   11,   40,   52,   58,   40,   43,
     -23,  -18,    1,   14,   14,   26,    9,   24,
     -25,  -17,   -5,    0,    4,   11,    0,   -1,
     -25,  -11,  -41,   -5,  -10,  -14,    7,  -11,
     -23,  -19,   -6,   -6,   -4,   -9,  -17,  -20,
     -13,  -22,  -20,  -10,  -17,  -25,  -29,  -24
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

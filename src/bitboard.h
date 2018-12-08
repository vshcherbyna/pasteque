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

#ifndef BITBOARD_H
#define BITBOARD_H

#include "pasteque.h"

pasteque_namespace_begin

typedef unsigned long long bitboard;

enum
{
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

enum
{
    WHITE_PIECE = 0,
    BLACK_PIECE = 8
};

enum
{
    EMPTY   = 0,
    KNIGHT  = 1,
    PAWN    = 2,
    KING    = 3,
    BISHOP  = 5,
    ROOK    = 6,
    QUEEN   = 7
};

enum
{
    WHITE_KNIGHT    = WHITE_PIECE | KNIGHT, // 00000001  1
    WHITE_PAWN      = WHITE_PIECE | PAWN,   // 00000010  2
    WHITE_KING      = WHITE_PIECE | KING,   // 00000011  3
    WHITE_BISHOP    = WHITE_PIECE | BISHOP, // 00000101  5
    WHITE_ROOK      = WHITE_PIECE | ROOK,   // 00000110  6
    WHITE_QUEEN     = WHITE_PIECE | QUEEN,  // 00000111  7
    BLACK_KNIGHT    = BLACK_PIECE | KNIGHT, // 00001001  9
    BLACK_PAWN      = BLACK_PIECE | PAWN,   // 00001010 10
    BLACK_KING      = BLACK_PIECE | KING,   // 00001011 11
    BLACK_ROOK      = BLACK_PIECE | ROOK,   // 00001101 13
    BLACK_QUEEN     = BLACK_PIECE | QUEEN   // 00001111 15
};

#define bit_set(number, bit) number |= 1ULL << bit
#define bit_unset(number, bit) number &= (~(1ULL << bit))

static int firstOne(bitboard bitmap)
{
    // De Bruijn Multiplication, see http://chessprogramming.wikispaces.com/BitScan
    // don't use this if bitmap = 0

    static const int INDEX64[64] = {
        63,  0, 58,  1, 59, 47, 53,  2,
        60, 39, 48, 27, 54, 33, 42,  3,
        61, 51, 37, 40, 49, 18, 28, 20,
        55, 30, 34, 11, 43, 14, 22,  4,
        62, 57, 46, 52, 38, 26, 32, 41,
        50, 36, 17, 19, 29, 10, 13, 21,
        56, 45, 25, 31, 35, 16,  9, 12,
        44, 24, 15,  8, 23,  7,  6,  5 };

    static const bitboard DEBRUIJN64 = 0x07EDD5E59A4E28C2;

    // here you would get a warming: "unary minus operator applied to unsigned type",
    // that's intended and OK so I'll disable it

    return INDEX64[((bitmap & -bitmap) * DEBRUIJN64) >> 58];
}

pasteque_namespace_end
#endif // BITBOARD_H

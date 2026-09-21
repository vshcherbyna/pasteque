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

#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

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
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE
};

enum
{
    WHITE = 0,
    BLACK = 1
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
    BLACK_BISHOP    = BLACK_PIECE | BISHOP, // 00001101 13
    BLACK_ROOK      = BLACK_PIECE | ROOK,   // 00001110 14
    BLACK_QUEEN     = BLACK_PIECE | QUEEN   // 00001111 15
};

enum
{
    WHITE_SHORT    = 1,
    WHITE_LONG   = 2,
    BLACK_SHORT    = 4,
    BLACK_LONG   = 8
};

#define bit_of(square) (1ULL << (square))

#define bit_set(number, bit) number |= 1ULL << bit
#define bit_unset(number, bit) number &= (~(1ULL << bit))

#define piece_color(piece) (((piece) >> 3) & 1)

#define square_file(square) ((square) & 7)
#define square_rank(square) ((square) >> 3)
#define square_of(file, rank) (((rank) << 3) | (file))

#define piece_of(type, color) ((type) | ((color) << 3))

#define rank_mask(rank) (0xffULL << ((rank) << 3))
#define file_mask(file) (0x0101010101010101ULL << (file))

inline int firstOne(bitboard bitmap) {

#if defined(_MSC_VER)
    unsigned long index;
    _BitScanForward64(&index, bitmap);
    return static_cast<int>(index);
#else
    return __builtin_ctzll(bitmap);
#endif
}

inline int popCount(bitboard bitmap) {
#if defined(_MSC_VER)
    return static_cast<int>(__popcnt64(bitmap));
#else
    return __builtin_popcountll(bitmap);
#endif
}

inline int popFirstOne(bitboard & bitmap) {

    auto square = firstOne(bitmap);
    bitmap &= bitmap - 1;

    return square;
}

pasteque_namespace_end
#endif // BITBOARD_H

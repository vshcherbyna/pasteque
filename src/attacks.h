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

#ifndef ATTACKS_H
#define ATTACKS_H

#include "pasteque.h"
#include "bitboard.h"

#if defined(_BTYPE) && (_BTYPE == 1)
#if defined(__x86_64__) || defined(_M_X64)
#define PASTEQUE_PEXT 1
#endif
#endif

#if defined(PASTEQUE_PEXT)
#if _WIN32 || _WIN64
#include <immintrin.h>
#endif
#if __GNUC__
#include <x86intrin.h>
#endif
#endif

pasteque_namespace_begin

struct SliderKey {
#if defined(PASTEQUE_PEXT)
    bitboard    m_mask;
    bitboard *  m_attacks;
#else
    bitboard    m_mask;
    bitboard    m_key;
    bitboard *  m_attacks;
    unsigned    m_shift;
#endif
};

extern bitboard PAWN_ATTACKS[2][64];
extern bitboard KNIGHT_ATTACKS[64];
extern bitboard KING_ATTACKS[64];

extern SliderKey    ROOK_KEYS[64];
extern SliderKey    BISHOP_KEYS[64];

extern bitboard BETWEEN[64][64];
extern bitboard LINE[64][64];

void initAttacks();

inline bitboard rookAttacks(int square, bitboard occupied) {
#if defined(PASTEQUE_PEXT)
    const auto & key = ROOK_KEYS[square];

    return key.m_attacks[_pext_u64(occupied, key.m_mask)];
#else
    const auto & magic = ROOK_KEYS[square];

    return magic.m_attacks[((occupied & magic.m_mask) * magic.m_key) >> magic.m_shift];
#endif
}

inline bitboard bishopAttacks(int square, bitboard occupied) {
#if defined(PASTEQUE_PEXT)
    const auto & key = BISHOP_KEYS[square];

    return key.m_attacks[_pext_u64(occupied, key.m_mask)];
#else
    const auto & magic = BISHOP_KEYS[square];

    return magic.m_attacks[((occupied & magic.m_mask) * magic.m_key) >> magic.m_shift];
#endif
}

inline bitboard queenAttacks(int square, bitboard occupied) {
    return rookAttacks(square, occupied) | bishopAttacks(square, occupied);
}

inline bitboard pieceAttacks(unsigned char piece, int square, bitboard occupied) {

    switch (piece & 7) {
    case KNIGHT:    return KNIGHT_ATTACKS[square];
    case KING:      return KING_ATTACKS[square];
    case BISHOP:    return bishopAttacks(square, occupied);
    case ROOK:      return rookAttacks(square, occupied);
    case QUEEN:     return queenAttacks(square, occupied);
    }

    return 0;
}

pasteque_namespace_end
#endif // ATTACKS_H

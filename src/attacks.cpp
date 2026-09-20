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

#include "attacks.h"

pasteque_namespace_begin

bitboard PAWN_ATTACKS[2][64];
bitboard KNIGHT_ATTACKS[64];
bitboard KING_ATTACKS[64];

SliderKey ROOK_KEYS[64];
SliderKey BISHOP_KEYS[64];

bitboard BETWEEN[64][64];
bitboard LINE[64][64];

static bitboard ROOK_TABLE[102400];
static bitboard BISHOP_TABLE[5248];

static const int KNIGHT_DELTAS[8][2] = {
    { -2, -1 }, { -2, 1 }, { -1, -2 }, { -1, 2 },
    {  1, -2 }, {  1, 2 }, {  2, -1 }, {  2, 1 }
};

static const int KING_DELTAS[8][2] = {
    { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, -1 },
    {  0,  1 }, {  1, -1 }, { 1, 0 }, { 1,  1 }
};

static const int ROOK_DELTAS[4][2]   = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
static const int BISHOP_DELTAS[4][2] = { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };

static bool onBoard(int file, int rank) {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}

static bitboard stepAttacks(int square, const int deltas[8][2]) {

    bitboard attacks = 0;

    for (auto i = 0; i < 8; ++i) {
        auto file = square_file(square) + deltas[i][0];
        auto rank = square_rank(square) + deltas[i][1];

        if (onBoard(file, rank))
            bit_set(attacks, square_of(file, rank));
    }

    return attacks;
}

static bitboard slidingAttacks(int square, bitboard occupied, const int deltas[4][2]) {

    bitboard attacks = 0;

    for (auto i = 0; i < 4; ++i) {
        auto file = square_file(square) + deltas[i][0];
        auto rank = square_rank(square) + deltas[i][1];

        while (onBoard(file, rank)) {
            auto to = square_of(file, rank);

            bit_set(attacks, to);

            if (occupied & bit_of(to))
                break;

            file += deltas[i][0];
            rank += deltas[i][1];
        }
    }

    return attacks;
}

static bitboard slidingMask(int square, const int deltas[4][2]) {

    bitboard mask = 0;

    for (auto i = 0; i < 4; ++i) {
        auto file = square_file(square) + deltas[i][0];
        auto rank = square_rank(square) + deltas[i][1];

        while (onBoard(file + deltas[i][0], rank + deltas[i][1])) {
            bit_set(mask, square_of(file, rank));

            file += deltas[i][0];
            rank += deltas[i][1];
        }
    }

    return mask;
}

static bitboard s_randomSeed;

static bitboard random64() {
    s_randomSeed ^= s_randomSeed >> 12;
    s_randomSeed ^= s_randomSeed << 25;
    s_randomSeed ^= s_randomSeed >> 27;

    return s_randomSeed * 2685821657736338717ULL;
}

static bitboard sparseRandom() {
    return random64() & random64() & random64();
}

static void initSliderKeys(SliderKey magics[64], bitboard table[], const int deltas[4][2]) {

    static bitboard occupancies[4096],
                    references[4096],
                    used[4096];

    auto offset = 0;

    for (auto square = 0; square < 64; ++square) {

        auto & magic = magics[square];

        magic.m_mask    = slidingMask(square, deltas);
        magic.m_shift   = 64 - popCount(magic.m_mask);
        magic.m_attacks = table + offset;

        bitboard occupied = 0;
        auto size = 0;

        do {
            occupancies[size] = occupied;
            references[size]  = slidingAttacks(square, occupied, deltas);

            ++size;
            occupied = (occupied - magic.m_mask) & magic.m_mask;
        }
        while (occupied);

        offset += size;

        for (;;) {

            do
                magic.m_key = sparseRandom();
            while (popCount((magic.m_mask * magic.m_key) >> 56) < 6);

            for (auto i = 0; i < size; ++i)
                used[i] = 0;

            auto collision = false;

            for (auto i = 0; i < size && !collision; ++i) {
                auto index = static_cast<unsigned>((occupancies[i] * magic.m_key) >> magic.m_shift);

                if (used[index] == 0)
                    used[index] = references[i];
                else if (used[index] != references[i])
                    collision = true;
            }

            if (collision)
                continue;

            for (auto i = 0; i < size; ++i) {
                auto index = static_cast<unsigned>((occupancies[i] * magic.m_key) >> magic.m_shift);
                magic.m_attacks[index] = references[i];
            }

            break;
        }
    }
}

void initAttacks() {

    static auto initialised = false;

    if (initialised)
        return;

    initialised = true;

    for (auto square = 0; square < 64; ++square) {
        KNIGHT_ATTACKS[square] = stepAttacks(square, KNIGHT_DELTAS);
        KING_ATTACKS[square]   = stepAttacks(square, KING_DELTAS);

        auto file = square_file(square);
        auto rank = square_rank(square);

        bitboard white = 0,
                 black = 0;

        if (rank < 7) {
            if (file > 0) bit_set(white, square_of(file - 1, rank + 1));
            if (file < 7) bit_set(white, square_of(file + 1, rank + 1));
        }

        if (rank > 0) {
            if (file > 0) bit_set(black, square_of(file - 1, rank - 1));
            if (file < 7) bit_set(black, square_of(file + 1, rank - 1));
        }

        PAWN_ATTACKS[WHITE][square] = white;
        PAWN_ATTACKS[BLACK][square] = black;
    }

    s_randomSeed = 1070372;

    initSliderKeys(ROOK_KEYS, ROOK_TABLE, ROOK_DELTAS);
    initSliderKeys(BISHOP_KEYS, BISHOP_TABLE, BISHOP_DELTAS);

    for (auto from = 0; from < 64; ++from) {
        for (auto diagonal = 0; diagonal < 2; ++diagonal) {
            auto deltas = diagonal ? BISHOP_DELTAS : ROOK_DELTAS;
            auto reach  = slidingAttacks(from, 0, deltas);
            auto rest   = reach;

            while (rest) {
                auto to = popFirstOne(rest);

                BETWEEN[from][to] = slidingAttacks(from, bit_of(to), deltas) & slidingAttacks(to, bit_of(from), deltas);
                LINE[from][to] = (reach & slidingAttacks(to, 0, deltas)) | bit_of(from) | bit_of(to);
            }
        }
    }
}

pasteque_namespace_end

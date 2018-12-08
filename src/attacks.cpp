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

#include "attacks.h"

pasteque_namespace_begin

bitboard KNIGHT_ATTACKS[64];

const int RANKS[64] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8
};

const int FILES[64] = {
    1, 2, 3, 4, 5, 6, 7, 8,
    1, 2, 3, 4, 5, 6, 7, 8,
    1, 2, 3, 4, 5, 6, 7, 8,
    1, 2, 3, 4, 5, 6, 7, 8,
    1, 2, 3, 4, 5, 6, 7, 8,
    1, 2, 3, 4, 5, 6, 7, 8,
    1, 2, 3, 4, 5, 6, 7, 8,
    1, 2, 3, 4, 5, 6, 7, 8
};

bitboard BITSET000[64];
int BOARDINDEX[9][9]; // index 0 is not used, only 1..8.

void initAttacks()
{
    // BOARDINDEX can be used to translate [file][rank] to [square]
    for (int rank = 0; rank < 9; rank++)
        for (int file = 0; file < 9; file++)
            BOARDINDEX[file][rank] = (rank - 1) * 8 + file - 1;

    BITSET000[0] = 1;
    for (unsigned char i = 1; i < 64; i++) BITSET000[i] = BITSET000[i - 1] << 1;

    unsigned char   square,
        file,
        rank,
        afile,
        arank;

    // KNIGHT attacks;
    for (square = 0; square < 64; square++) {
        file = FILES[square];
        rank = RANKS[square];
        afile = file - 2; arank = rank + 1;
        if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
            KNIGHT_ATTACKS[square] |= BITSET000[BOARDINDEX[afile][arank]];
        afile = file - 1; arank = rank + 2;
        if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
            KNIGHT_ATTACKS[square] |= BITSET000[BOARDINDEX[afile][arank]];
        afile = file + 1; arank = rank + 2;
        if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
            KNIGHT_ATTACKS[square] |= BITSET000[BOARDINDEX[afile][arank]];
        afile = file + 2; arank = rank + 1;
        if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
            KNIGHT_ATTACKS[square] |= BITSET000[BOARDINDEX[afile][arank]];
        afile = file + 2; arank = rank - 1;
        if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
            KNIGHT_ATTACKS[square] |= BITSET000[BOARDINDEX[afile][arank]];
        afile = file + 1; arank = rank - 2;
        if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
            KNIGHT_ATTACKS[square] |= BITSET000[BOARDINDEX[afile][arank]];
        afile = file - 1; arank = rank - 2;
        if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
            KNIGHT_ATTACKS[square] |= BITSET000[BOARDINDEX[afile][arank]];
        afile = file - 2; arank = rank - 1;
        if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
            KNIGHT_ATTACKS[square] |= BITSET000[BOARDINDEX[afile][arank]];
    }
}

pasteque_namespace_end

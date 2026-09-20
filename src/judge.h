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

#ifndef JUDGE_H
#define JUDGE_H

#include "pasteque.h"
#include "bitboard.h"
#include "board.h"

pasteque_namespace_begin

struct Taper
{
    Taper() : opening{0}, closing{0} {}
    Taper(int o, int c) : opening{o}, closing{c} {}

    void operator+=(const Taper & other) { opening += other.opening; closing += other.closing; }
    void operator-=(const Taper & other) { opening -= other.opening; closing -= other.closing; }

    Taper operator-() const { return Taper(-opening, -closing); }

    int opening,
        closing;
};

enum
{
    EVEN_SCORE      = 0,
    MATE_SCORE      = 32000,
    HUGE_SCORE      = 32001,

    PHASE_MAX       = 24,
    TEMPO           = 14
};

extern Taper PIECE_SQUARE[16][64];

class Judge
{
public:
    static void init();
    static int evaluate(const Board & board);

public:
    static Taper pieceSquare(const Board & board);
    static int  phase(const Board & board);

private:
    static int  scale(const Board & board, int eval);
};

pasteque_namespace_end
#endif // JUDGE_H

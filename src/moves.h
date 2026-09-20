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

#ifndef MOVES_H
#define MOVES_H

#include "pasteque.h"
#include "move.h"
#include "board.h"

pasteque_namespace_begin

enum
{
    MOVE_LIMIT = 256
};

class Moves
{
public:
    Moves() : m_size{0} {}

public:
    void    generate(const Board & board);
    void    generateLegal(const Board & board);

public:
    int     size() const { return m_size; }
    Move    operator[](int index) const { return m_moves[index]; }
    Move &  operator[](int index) { return m_moves[index]; }

private:
    void    add(Move move) { m_moves[m_size++] = move; }
    void    addPromotions(int from, int to, int piece, int capture);

    void    generatePawns(const Board & board, unsigned char side, bitboard mask, bitboard checkers);
    void    generatePieces(const Board & board, unsigned char side, unsigned char type, bitboard mask);
    void    generateCastlings(const Board & board, unsigned char side);

private:
    Move    m_moves[MOVE_LIMIT];
    int     m_size;
};

pasteque_namespace_end
#endif // MOVES_H

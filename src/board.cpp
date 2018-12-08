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

#include "board.h"
#include "move.h"
#include "attacks.h"

pasteque_namespace_begin

extern bitboard KNIGHT_ATTACKS[64];
Board::Board()
{
    bit_set(m_white.m_rooks, A1);
    bit_set(m_white.m_rooks, H1);

    bit_set(m_white.m_knights, B1);
    bit_set(m_white.m_knights, G1);

    bit_set(m_white.m_bishops, C1);
    bit_set(m_white.m_bishops, F1);

    bit_set(m_white.m_queen, D1);
    bit_set(m_white.m_king, E1);

    for (auto i = 8; i <= 15; ++i)
        bit_set(m_white.m_pawns, i);

    bit_set(m_black.m_rooks, A8);
    bit_set(m_black.m_rooks, H8);

    bit_set(m_black.m_knights, B8);
    bit_set(m_black.m_knights, G8);

    bit_set(m_black.m_bishops, C8);
    bit_set(m_black.m_bishops, F8);

    bit_set(m_black.m_queen, D8);
    bit_set(m_black.m_king, C8);

    for (auto i = 48; i <= 55; ++i)
        bit_set(m_black.m_pawns, i);

    initAttacks();
    bitboard targetBitmap = ~m_white.getAllPieces();
    Move move;

    move.piece(WHITE_KNIGHT);
    bitboard tempPiece = m_white.m_knights;

    while (tempPiece)
    {
        bitboard from = firstOne(tempPiece);
        bitboard tempMove = KNIGHT_ATTACKS[from] & targetBitmap;

        while (tempMove)
        {
            bitboard to = firstOne(tempMove);
            move.to(to);
            //move.capture(board.square[to]);
        }
    }
}

pasteque_namespace_end

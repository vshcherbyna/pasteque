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

#include "moves.h"
#include "attacks.h"

pasteque_namespace_begin

extern bitboard KNIGHT_ATTACKS[64];

Moves::Moves(Board board) : m_board(board)
{

}

Moves::listOfMoves Moves::allMoves()
{
    Moves::listOfMoves moves;

    moveKnights();

    return {};
}

Moves::listOfMoves Moves::moveKnights()
{
    bitboard tempMove;
    Move move;
    move.piece(BLACK_KNIGHT);
    auto tempPiece = m_board.m_black.m_knights;
    auto targetBitmap = ~m_board.m_black.getAllPieces();
    while (tempPiece)
    {
        auto from = firstOne(tempPiece);
        move.from(from);
        tempMove = KNIGHT_ATTACKS[from] & targetBitmap;
        while (tempMove)
        {
            auto to = firstOne(tempMove);
            /*move.setTosq(to);
            move.setCapt(board.square[to]);
            board.moveBuffer[index++].moveInt = move.moveInt;
            tempMove ^= BITSET[to];*/
        }
        //tempPiece ^= BITSET[from];
    }
    return {};
}
pasteque_namespace_end

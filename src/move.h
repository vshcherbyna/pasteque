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

#ifndef MOVE_H
#define MOVE_H

#include <string>

#include "pasteque.h"
#include "bitboard.h"

pasteque_namespace_begin

/*
    m_move contains move information in the following format:
    [6 bits FROM square] [6 bits TO square] [4 bits PIECE] [4 bits CAPTURE] [4 bits PROMOTION] [2 bits FLAGS]
*/

enum
{
    MOVE_CASTLING  = 1 << 24,
    MOVE_ENPASSANT = 1 << 25
};

class Move
{
public:
    Move() : m_move{0} {}
    explicit Move(unsigned int move) : m_move{move} {}

    Move(unsigned int from, unsigned int to, unsigned int piece) :
        m_move{from | (to << 6) | (piece << 12)} {}

    Move(unsigned int from, unsigned int to, unsigned int piece, unsigned int capture) :
        m_move{from | (to << 6) | (piece << 12) | (capture << 16)} {}

    Move(unsigned int from, unsigned int to, unsigned int piece, unsigned int capture, unsigned int promotion) :
        m_move{from | (to << 6) | (piece << 12) | (capture << 16) | (promotion << 20)} {}

    Move(unsigned int from, unsigned int to, unsigned int piece, unsigned int capture, unsigned int promotion, unsigned int flags) :
        m_move{from | (to << 6) | (piece << 12) | (capture << 16) | (promotion << 20) | flags} {}

public:
    void from(unsigned char);
    void to(unsigned char);
    void piece(unsigned char);
    void capture(unsigned char);
    void promotion(unsigned char);

public:
    int  getFrom() const        { return m_move & 0x3f; }
    int  getTo() const          { return (m_move >> 6) & 0x3f; }
    int  getPiece() const       { return (m_move >> 12) & 0x0f; }
    int  getCapture() const     { return (m_move >> 16) & 0x0f; }
    int  getPromotion() const   { return (m_move >> 20) & 0x0f; }

    bool isCastling() const     { return (m_move & MOVE_CASTLING) != 0; }
    bool isEnPassant() const    { return (m_move & MOVE_ENPASSANT) != 0; }

    std::string toString() const;

    operator int() const {return m_move;}

private:
    unsigned int m_move;
};

pasteque_namespace_end
#endif // MOVE_H

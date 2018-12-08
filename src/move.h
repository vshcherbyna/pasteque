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

#include "pasteque.h"

pasteque_namespace_begin

/*
    m_move contains move information in the following format:
    [6 bits FROM square] [6 bits TO square] [4 bits PIECE] [4 bits CAPTURE] [4 bits PROMOTION]
*/

class Move
{
public:
    Move();

public:
    void from(unsigned char square_from);
    void to(unsigned char square_to);
    void piece(unsigned char piece);
    void capture(unsigned char capture);

private:
    void reset();

private:
    unsigned int m_move;
};

pasteque_namespace_end
#endif // MOVE_H

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

#ifndef SIDE_H
#define SIDE_H

#include "bitboard.h"

pasteque_namespace_begin

class Side
{
public:
    Side();

public:
    bitboard getAllPieces();

public:
    bitboard    m_king,
        m_queen,
        m_rooks,
        m_bishops,
        m_knights,
        m_pawns;
};

pasteque_namespace_end
#endif // SIDE_H

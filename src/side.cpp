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

#include "side.h"

pasteque_namespace_begin

Side::Side()
{
    m_king = 0;
    m_queen = 0;
    m_rooks = 0;
    m_bishops = 0;
    m_knights = 0;
    m_pawns = 0;
}

bitboard Side::getAllPieces()
{
    return (m_king | m_queen | m_rooks | m_bishops | m_knights | m_pawns);
}

pasteque_namespace_end

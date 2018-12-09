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

#include "move.h"

pasteque_namespace_begin

Move::Move() : m_move{0}
{
}

void Move::from(unsigned char from)
{
    m_move &= 0xffffffc0;
    m_move |= (from & 0x0000003f);
}

void Move::to(unsigned char to)
{
    m_move &= 0xfffff03f;
    m_move |= (to & 0x0000003f) << 6;
}

void Move::piece(unsigned char piece)
{
    m_move &= 0xffff0fff;
    m_move |= (piece & 0x0000000f) << 12;
}

void Move::capture(unsigned char capture)
{
    m_move &= 0xfff0ffff;
    m_move |= (capture & 0x0000000f) << 16;
}

pasteque_namespace_end

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

#ifndef BOARD_H
#define BOARD_H

#include "pasteque.h"
#include "side.h"

pasteque_namespace_begin

class Board
{
public:
    Board();
private:
    Side m_white;
    Side m_black;
};

pasteque_namespace_end
#endif // BOARD_H

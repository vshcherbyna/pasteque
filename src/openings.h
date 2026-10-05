/*
*  pastèque - uci chess engine
*
*  Copyright (C) 2018-2026 Volodymyr Shcherbyna <volodymyr@shcherbyna.com>
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

#ifndef OPENINGS_H
#define OPENINGS_H

#include <string>
#include <vector>

#include "pasteque.h"

pasteque_namespace_begin

enum
{
    OPENINGS_COUNT = 1000,
    OPENINGS_PLIES = 8
};

void openingPositions(std::vector<std::string> & positions, int count, int plies, unsigned long long seed);
int  openings(int count, int plies, unsigned long long seed);

pasteque_namespace_end
#endif // OPENINGS_H

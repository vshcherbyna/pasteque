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

#ifndef LEARN_H
#define LEARN_H

#include <vector>

#include "pasteque.h"

pasteque_namespace_begin

enum
{
    LEARN_GAMES   = 1000,
    LEARN_NODES   = 5000,
    LEARN_KEEP    = 20,
    LEARN_LIMIT   = 600,
    LEARN_ROUNDS  = 40,
    LEARN_THREADS = 256
};

struct LearnRow
{
    int         material[5];

    long long   score,
                seen;
};

void learnRows(std::vector<LearnRow> & rows, int games, int nodes, unsigned long long seed, int threads);
bool learnFit(const std::vector<LearnRow> & rows, double values[6]);
int  learn(int games, int nodes, unsigned long long seed, int threads);

pasteque_namespace_end
#endif // LEARN_H

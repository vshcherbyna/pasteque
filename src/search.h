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

#ifndef SEARCH_H
#define SEARCH_H

#include "pasteque.h"
#include "board.h"
#include "moves.h"

pasteque_namespace_begin

enum
{
    PLY_LIMIT = 128
};

typedef void (*Watcher)(int depth, int score, unsigned long long nodes, unsigned long long msec, Move best);

class Search
{
public:
    Search();

public:
    Move                bestMove(Board & board, int depth);
    void                setWatcher(Watcher watcher) { m_watcher = watcher; }

public:
    unsigned long long  getNodes() const { return m_nodes; }
    int                 getScore() const { return m_score; }

private:
    int                 alphaBeta(Board & board, int alpha, int beta, int depth, int ply);
    int                 quiescence(Board & board, int alpha, int beta, int ply);
    void                order(Moves & moves);

private:
    unsigned long long  m_nodes;
    int                 m_score;
    Watcher             m_watcher;
};

pasteque_namespace_end
#endif // SEARCH_H

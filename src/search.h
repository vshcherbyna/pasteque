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

#ifndef SEARCH_H
#define SEARCH_H

#include <vector>

#include "pasteque.h"
#include "board.h"
#include "moves.h"
#include "clock.h"

pasteque_namespace_begin

enum
{
    PLY_LIMIT = 128,
    POLL_MASK = 2047
};

enum
{
    ORDER_CAPTURE   = 100000,
    ORDER_PROMOTION = 200000,
    ORDER_VICTIM    = 32,
    ORDER_KILLER    = 90000,
    ORDER_MERIT     = 80000,
    ORDER_HASH      = 300000
};

enum
{
    HASH_DEFAULT = 4,
    HASH_LEAST   = 1,
    HASH_MOST    = 1024
};

enum
{
    HASH_HORIZON = 20
};

enum
{
    NULL_DEPTH  = 3,
    NULL_REDUCE = 2
};

enum
{
    REDUCE_DEPTH      = 3,
    REDUCE_MOVES      = 3,
    REDUCE_MORE_DEPTH = 6,
    REDUCE_MORE_MOVES = 6
};

enum
{
    HASH_EXACT = 1,
    HASH_UPPER = 2,
    HASH_LOWER = 3
};

struct HashEntry
{
    unsigned int    check,
                    age;

    int             move;

    short           score;

    signed char     depth;
    unsigned char   bound;
};

typedef void (*Watcher)(int depth, int score, unsigned long long nodes, unsigned long long msec, Move best);

class Search
{
public:
    Search();

public:
    Move                bestMove(Board & board, int depth);
    Move                bestMove(Board & board, const Clock & clock);
    void                setWatcher(Watcher watcher) { m_watcher = watcher; }
    void                setHash(int megabytes);

public:
    unsigned long long  getNodes() const { return m_nodes; }
    int                 getScore() const { return m_score; }
    size_t              getSlots() const { return m_slots; }

#if defined(UNIT_TEST)
public:
#else
private:
#endif
    Move                deepen(Board & board, int depth, Instant started, unsigned int soft);
    int                 alphaBeta(Board & board, int alpha, int beta, int depth, int ply);
    int                 quiescence(Board & board, int alpha, int beta, int ply);
    void                order(Moves & moves, int ply, Move favoured);
    int                 reduce(Move move, int depth, int played, bool checked, bool checking);
    bool                verify(int score, int alpha, int beta, int reduction);
    bool                nullAllowed(const Board & board, int depth, int ply, int beta, bool checked);
    void                reward(Move move, int depth, int ply);
    bool                probe(stamp key, int fifty, int depth, int ply, int alpha, int beta, int & score, Move & favoured);
    void                store(stamp key, int fifty, int depth, int ply, int score, int bound, Move move);
    void                pollClock();

private:
    unsigned long long  m_nodes,
                        m_quota;

    int                 m_score;
    Watcher             m_watcher;

    Instant             m_deadline;
    bool                m_timed,
                        m_aborted;

#if defined(UNIT_TEST)
public:
#else
private:
#endif
    Move                m_killers[PLY_LIMIT][2] = {};
    bool                m_nulled[PLY_LIMIT] = {};
    int                 m_merit[16][64] = {};

    std::vector<HashEntry>  m_hash;
    size_t              m_slots = 0;
    unsigned int        m_age = 1;
};

pasteque_namespace_end
#endif // SEARCH_H

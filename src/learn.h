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

#include <string>
#include <vector>

#include "pasteque.h"

pasteque_namespace_begin

class Board;

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

enum
{
    GRID_TERMS   = 6 * 64 + 1,
    GRID_ROUNDS  = 30,
    GRID_SAMPLES = 20000000,
    GRID_TRIALS  = 5,
    GRID_HOLDOUT = 5,
    GRID_BLOCKS  = 64
};

//
//  One retained position: the squares it occupies, and how often the games that reached it were
//  won. score and seen carry the same meaning they do in LearnRow, so a target of any fraction
//  can be stated exactly. withheld is assigned by opening index and is the same for every
//  retained position from that game
//

struct GridSample
{
    short           term[32];

    unsigned short  score,
                    seen;

    unsigned char   terms;

    bool            withheld = false;
};

struct GridTally
{
    int                 played,
                        concluded;

    unsigned long long  spent;
};

extern const unsigned char GRID_KINDS[6];

void gridTerms(const Board & board, GridSample & sample);
void gridSamples(std::vector<GridSample> & samples, int games, int nodes, unsigned long long seed, int threads, GridTally & tally);
bool   gridFit(const std::vector<GridSample> & samples, double values[GRID_TERMS], double strength, int threads, double & settled);
double gridLoss(const std::vector<GridSample> & samples, const double values[GRID_TERMS]);
int gridVictim(const int value[5], int & narrowest, int & spread);

std::string gridSheet(int games, int nodes, unsigned long long seed);
int    grid(int games, int nodes, unsigned long long seed, int threads, double strength);

pasteque_namespace_end
#endif // LEARN_H

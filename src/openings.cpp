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

#include <iostream>
#include <unordered_set>

#include "openings.h"
#include "board.h"
#include "moves.h"

pasteque_namespace_begin

static const unsigned long long OPENINGS_SEED = 0x2b1cf0a9e4d35b87ULL;

static unsigned long long nextChoice(unsigned long long & seed) {
    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;

    return seed * 0x2545f4914f6cdd1dULL;
}

static bool scatter(Board & board, int plies, unsigned long long & seed) {

    for (auto ply = 0; ply < plies; ++ply) {
        Moves moves;
        moves.generateLegal(board);

        if (!moves.size())
            return false;

        auto   pick = static_cast<int>(nextChoice(seed) % static_cast<unsigned long long>(moves.size()));
        Rewind undo;

        board.doMove(moves[pick], undo);
    }

    Moves moves;
    moves.generateLegal(board);

    return moves.size() != 0;
}

void openingPositions(std::vector<std::string> & positions, int count, int plies, unsigned long long seed) {

    if (count < 1)
        count = OPENINGS_COUNT;

    if (plies < 1)
        plies = OPENINGS_PLIES;

    if (!seed)
        seed = OPENINGS_SEED;

    std::unordered_set<stamp> seen;

    long long attempts = 0,
              ceiling  = static_cast<long long>(count) * 1000;

    positions.clear();

    while (static_cast<int>(positions.size()) < count && attempts < ceiling) {
        ++attempts;

        Board board;

        if (!scatter(board, plies, seed))
            continue;

        if (!seen.insert(board.getStamp()).second)
            continue;

        positions.push_back(board.fen());
    }
}

int openings(int count, int plies, unsigned long long seed) {

    std::vector<std::string> positions;

    openingPositions(positions, count, plies, seed);

    std::string batch;

    for (const auto & position : positions) {
        batch += position;
        batch += '\n';

        if (batch.size() >= 1 << 16) {
            std::cout << batch;
            batch.clear();
        }
    }

    std::cout << batch;

    return positions.empty() ? 1 : 0;
}

pasteque_namespace_end

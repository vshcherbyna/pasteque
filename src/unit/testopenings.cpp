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

#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "../openings.h"
#include "../board.h"
#include "../moves.h"

pasteque_namespace_begin
namespace unit
{

TEST(Openings, Deterministic) {
    std::vector<std::string> first,
                             second;

    openingPositions(first, 64, 8, 1234);
    openingPositions(second, 64, 8, 1234);

    EXPECT_EQ(first.size(), 64u);
    EXPECT_EQ(first, second);
}

TEST(Openings, SeedChangesTheSet) {
    std::vector<std::string> first,
                             second;

    openingPositions(first, 64, 8, 1234);
    openingPositions(second, 64, 8, 5678);

    EXPECT_NE(first, second);
}

TEST(Openings, EveryPositionIsPlayable) {
    std::vector<std::string> positions;

    openingPositions(positions, 128, 8, 99);

    ASSERT_EQ(positions.size(), 128u);

    for (const auto & fen : positions) {
        Board b{};

        ASSERT_TRUE(b.setFen(fen)) << fen;

        EXPECT_EQ(b.fen(), fen) << fen;
        EXPECT_EQ(b.getMoveNumber(), 5u) << fen;

        Moves moves;
        moves.generateLegal(b);

        EXPECT_NE(moves.size(), 0) << fen;
    }
}

TEST(Openings, PositionsAreDistinct) {
    std::vector<std::string> positions;

    openingPositions(positions, 256, 10, 7);

    ASSERT_EQ(positions.size(), 256u);

    std::set<std::string> seen(positions.begin(), positions.end());

    EXPECT_EQ(seen.size(), positions.size());
}

TEST(Openings, DefaultsFillIn) {
    std::vector<std::string> positions;

    openingPositions(positions, 0, 0, 0);

    EXPECT_EQ(positions.size(), static_cast<size_t>(OPENINGS_COUNT));

    Board b{};

    ASSERT_TRUE(b.setFen(positions.front()));
    EXPECT_EQ(b.getMoveNumber(), 1u + OPENINGS_PLIES / 2);
}

}
pasteque_namespace_end

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

#include <cmath>
#include <vector>

#include "../learn.h"

pasteque_namespace_begin
namespace unit
{

TEST(Learn_fit, RecoversKnownWeights) {
    static const double TRUTH[6] = { 0.30, 0.90, 1.00, 1.60, 2.80, -0.05 };

    std::vector<LearnRow> rows;

    for (auto pawns = -3; pawns <= 3; ++pawns)
        for (auto knights = -1; knights <= 1; ++knights)
            for (auto bishops = -1; bishops <= 1; ++bishops)
                for (auto rooks = -1; rooks <= 1; ++rooks)
                    for (auto queens = -1; queens <= 1; ++queens)
                    {
                        LearnRow row;

                        row.material[0] = pawns;
                        row.material[1] = knights;
                        row.material[2] = bishops;
                        row.material[3] = rooks;
                        row.material[4] = queens;

                        auto sum = TRUTH[5] + TRUTH[0] * pawns + TRUTH[1] * knights
                                 + TRUTH[2] * bishops + TRUTH[3] * rooks + TRUTH[4] * queens;

                        auto chance = 1.0 / (1.0 + std::exp(-sum));

                        row.seen  = 2000;
                        row.score = static_cast<long long>(std::floor(2.0 * row.seen * chance + 0.5));

                        rows.push_back(row);
                    }

    double got[6];

    ASSERT_TRUE(learnFit(rows, got));

    for (auto k = 0; k < 6; ++k)
        EXPECT_NEAR(got[k], TRUTH[k], 0.02) << "weight " << k;
}

TEST(Learn_fit, Negative) {
    std::vector<LearnRow> empty;
    double                got[6];

    EXPECT_FALSE(learnFit(empty, got));
}

TEST(Learn_rows, Deterministic) {
    std::vector<LearnRow> first,
                          second;

    learnRows(first, 6, 600, 4242, 1);
    learnRows(second, 6, 600, 4242, 1);

    ASSERT_FALSE(first.empty());
    ASSERT_EQ(first.size(), second.size());

    for (size_t i = 0; i < first.size(); ++i)
    {
        for (auto k = 0; k < 5; ++k)
            EXPECT_EQ(first[i].material[k], second[i].material[k]);

        EXPECT_EQ(first[i].score, second[i].score);
        EXPECT_EQ(first[i].seen, second[i].seen);
    }
}

TEST(Learn_rows, ThreadCountDoesNotChangeTheRows) {

    //
    //  A game depends on nothing but its opening and the counts are only summed, so more
    //  threads may only make the run shorter, never different
    //

    std::vector<LearnRow> alone,
                          crowd;

    learnRows(alone, 24, 600, 909, 1);
    learnRows(crowd, 24, 600, 909, 4);

    ASSERT_FALSE(alone.empty());
    ASSERT_EQ(alone.size(), crowd.size());

    for (size_t i = 0; i < alone.size(); ++i) {
        for (auto k = 0; k < 5; ++k)
            EXPECT_EQ(alone[i].material[k], crowd[i].material[k]) << "row " << i;

        EXPECT_EQ(alone[i].score, crowd[i].score) << "row " << i;
        EXPECT_EQ(alone[i].seen, crowd[i].seen) << "row " << i;
    }
}

TEST(Learn_rows, Sane) {
    std::vector<LearnRow> rows;

    learnRows(rows, 8, 600, 77, 1);

    ASSERT_FALSE(rows.empty());

    long long seen = 0;

    for (const auto & row : rows)
    {
        EXPECT_GT(row.seen, 0);
        EXPECT_GE(row.score, 0);
        EXPECT_LE(row.score, 2 * row.seen);

        for (auto k = 0; k < 5; ++k)
        {
            EXPECT_GE(row.material[k], -16);
            EXPECT_LE(row.material[k], 16);
        }

        seen += row.seen;
    }

    EXPECT_LE(seen, static_cast<long long>(8) * LEARN_KEEP);
}

}
pasteque_namespace_end

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

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "../learn.h"
#include "../board.h"
#include "../judge.h"

using namespace pasteque;

//
//  A term list has to describe the very sum the evaluation computes, or the fit would be shaping
//  something other than what the engine plays with. Two things here can silently be the wrong way
//  round, and neither is visible while the tables are flat: the mirror, because both sides index
//  one table from opposite ends, and the side flip. So a table that differs on every square is
//  installed for the duration, and the positions are chosen with material imbalance
//

namespace {

int planted(int slot, int entry) {

    return 1000 * slot + entry + 1;
}

}

TEST(Grid_terms, ReproduceThePieceSquareSum) {

    Judge::init();

    Taper keep[16][64];

    for (auto piece = 0; piece < 16; ++piece)
        for (auto square = 0; square < 64; ++square)
            keep[piece][square] = PIECE_SQUARE[piece][square];

    //
    //  Laid out the way the evaluation lays it out, and deliberately not the way a term is
    //  built, so that the two have to agree rather than share a mistake
    //

    for (auto row = 0; row < 8; ++row)
        for (auto file = 0; file < 8; ++file) {
            auto entry = row * 8 + file;
            auto white = square_of(file, 7 - row);
            auto black = square_of(file, row);

            for (auto slot = 0; slot < 6; ++slot) {
                Taper score(planted(slot, entry), planted(slot, entry));

                PIECE_SQUARE[piece_of(GRID_KINDS[slot], WHITE)][white] = score;
                PIECE_SQUARE[piece_of(GRID_KINDS[slot], BLACK)][black] = score;
            }
        }

    const char * const positions[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1",
        "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w - - 4 4",
        "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1",
        "4k3/4p3/8/8/8/8/8/4K3 b - - 0 1",
        "r3k3/8/8/8/8/8/8/4K3 b - - 0 1",
        "4k3/8/8/3N4/8/8/8/4K3 w - - 0 1",
        "4k3/8/8/4n3/8/8/8/4K3 b - - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "6k1/5ppp/8/8/8/8/PPPQ4/K7 b - - 0 1"
    };

    for (auto position : positions) {
        Board board;

        ASSERT_TRUE(board.setFen(position)) << position;

        GridSample sample;

        gridTerms(board, sample);

        EXPECT_EQ(popCount(board.getAllPieces()), sample.terms) << position;

        auto sum = 0;

        for (auto i = 0; i < sample.terms; ++i) {
            auto term  = sample.term[i];
            auto slot  = (term > 0) ? term - 1 : -term - 1;
            auto taken = (term > 0) ? 1 : -1;

            sum += taken * planted(slot / 64, slot % 64);
        }

        auto flip = (board.getSide() == WHITE) ? 1 : -1;

        EXPECT_EQ(flip * Judge::pieceSquare(board).opening, sum) << position;
    }

    for (auto piece = 0; piece < 16; ++piece)
        for (auto square = 0; square < 64; ++square)
            PIECE_SQUARE[piece][square] = keep[piece][square];
}

TEST(Grid_terms, AgreeWithTheClosingTable) {

    Judge::init();

    //
    //  grid fits one table per piece and writes it to both phases, so the two have to hold the
    //  same number. A fit that was read back into only one of them would leave the taper live
    //  and this test says so
    //

    for (auto kind = 0; kind < 6; ++kind)
        for (auto square = 0; square < 64; ++square) {
            auto piece = piece_of(GRID_KINDS[kind], WHITE);

            EXPECT_EQ(PIECE_SQUARE[piece][square].opening, PIECE_SQUARE[piece][square].closing)
                << "kind " << kind << " square " << square;
        }
}

namespace {

unsigned long long roll(unsigned long long & state) {

    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;

    return state * 0x2545f4914f6cdd1dULL;
}

double known(int slot, int entry) {

    static const double BASE[6] = { 0.29, 0.64, 0.84, 1.19, 2.77, 0.0 };

    auto file = entry % 8;
    auto row  = entry / 8;
    auto near = 7.0 - std::fabs(file - 3.5) * 2.0 - std::fabs(row - 3.5) * 2.0;

    return BASE[slot] + 0.02 * near;
}

}

//
//  Recovery on manufactured data, where the answer is known. score and seen state the target
//  exactly, so this measures the solver rather than the noise in a hard label
//

TEST(Grid_fit, RecoversKnownTables) {

    unsigned long long state = 0x9e3779b97f4a7c15ULL;

    std::vector<GridSample> samples;

    for (auto round = 0; round < 6000; ++round) {
        GridSample sample;

        sample.terms = 0;

        auto count = 6 + static_cast<int>(roll(state) % 20);
        auto sum   = 0.0;

        for (auto i = 0; i < count; ++i) {
            auto slot  = static_cast<int>(roll(state) % 6);
            auto entry = static_cast<int>(roll(state) % 64);
            auto taken = (roll(state) & 1) ? 1 : -1;

            sum += taken * known(slot, entry);

            sample.term[sample.terms++] = static_cast<short>(taken * (slot * 64 + entry + 1));
        }

        auto chance = 1.0 / (1.0 + std::exp(-sum));

        //
        //  seen is the denominator, so twice it is the widest score, and any chance in between
        //  is stated to within half a unit
        //

        sample.seen  = 2000;
        sample.score = static_cast<unsigned short>(std::floor(chance * 2.0 * sample.seen + 0.5));

        samples.push_back(sample);
    }

    std::vector<double> values(static_cast<size_t>(GRID_TERMS));

    ASSERT_TRUE(gridFit(samples, values.data(), 1.0));

    //
    //  The king is the one table the games cannot place: shifting every square of it by the same
    //  amount changes no evaluation at all, so only its shape is compared
    //

    for (auto slot = 0; slot < 6; ++slot) {
        auto want = 0.0;
        auto got  = 0.0;

        for (auto entry = 0; entry < 64; ++entry) {
            want += known(slot, entry);
            got  += values[static_cast<size_t>(slot) * 64 + entry];
        }

        want /= 64.0;
        got  /= 64.0;

        //  a gtest assertion expands to an if, so the guard needs its own braces
        if (slot != 5) {
            EXPECT_NEAR(want, got, 0.08) << "mean of slot " << slot;
        }

        auto apart = 0.0;

        for (auto entry = 0; entry < 64; ++entry) {
            auto mine = values[static_cast<size_t>(slot) * 64 + entry] - got;

            apart = std::max(apart, std::fabs((known(slot, entry) - want) - mine));
        }

        EXPECT_LT(apart, 0.05) << "shape of slot " << slot;
    }
}

TEST(Grid_fit, Negative) {

    std::vector<GridSample> samples;
    std::vector<double>     values(static_cast<size_t>(GRID_TERMS));

    EXPECT_FALSE(gridFit(samples, values.data(), 1.0));

    for (auto k = 0; k < static_cast<int>(GRID_TERMS); ++k)
        EXPECT_EQ(0.0, values[static_cast<size_t>(k)]);
}

//
//  A result that only ever reached stdout depended on the shell getting a redirection right, and
//  three shells get it three different ways. The sheet is the artifact; stdout is a courtesy
//

TEST(Grid_sheet, RecordsTheRunToAFile) {

    auto name = gridSheet(40, 400, 20261107);

    std::remove(name.c_str());

    std::ostringstream output;
    auto previous = std::cout.rdbuf(output.rdbuf());
    auto result = grid(40, 400, 20261107, 2, 16.0);
    std::cout.rdbuf(previous);

    ASSERT_EQ(0, result);

    std::ifstream file(name);

    ASSERT_TRUE(file.good()) << name;

    std::stringstream kept;

    kept << file.rdbuf();
    file.close();

    auto text = kept.str();

    //  the sheet names the build that generated the games, and the arguments that reproduce them

    EXPECT_NE(std::string::npos, text.find(ENGINE_VERSION)) << text;
    EXPECT_NE(std::string::npos, text.find("games 40, nodes 400, seed 20261107")) << text;
    EXPECT_NE(std::string::npos, text.find("PAWN_OPENING[64]")) << text;
    EXPECT_NE(std::string::npos, text.find("KING_CLOSING[64]")) << text;

    //  and the thread count is absent on purpose, so two runs of it compare byte for byte

    EXPECT_EQ(std::string::npos, text.find("threads")) << text;

    //
    //  What reached the console is what reached the file, and then one line more: the console is
    //  told where the sheet went, which the sheet itself has no business repeating
    //

    EXPECT_EQ(static_cast<size_t>(0), output.str().find(text));
    EXPECT_NE(std::string::npos, output.str().find("written to " + name));
    EXPECT_EQ(std::string::npos, text.find("written to"));

    std::remove(name.c_str());
}

TEST(Grid_sheet, NamesTheBuildAndTheArguments) {

    auto name = gridSheet(100000, 5000, 20261107);

    EXPECT_NE(std::string::npos, name.find(ENGINE_VERSION)) << name;
    EXPECT_NE(std::string::npos, name.find("100000")) << name;
    EXPECT_NE(std::string::npos, name.find("5000")) << name;
    EXPECT_NE(std::string::npos, name.find("20261107")) << name;

    //  a refit on the same seed by a newer build must not overwrite its predecessor

    EXPECT_NE(name, gridSheet(100000, 5000, 20261108));
}

TEST(Grid_samples, Deterministic) {

    std::vector<GridSample> one,
                            two;

    gridSamples(one, 12, 400, 20261006, 1);
    gridSamples(two, 12, 400, 20261006, 1);

    ASSERT_FALSE(one.empty());
    ASSERT_EQ(one.size(), two.size());

    for (size_t i = 0; i < one.size(); ++i) {
        ASSERT_EQ(one[i].terms, two[i].terms) << i;
        ASSERT_EQ(one[i].score, two[i].score) << i;
        ASSERT_EQ(one[i].seen,  two[i].seen)  << i;
        ASSERT_EQ(one[i].withheld, two[i].withheld) << i;

        for (auto k = 0; k < one[i].terms; ++k)
            ASSERT_EQ(one[i].term[k], two[i].term[k]) << i << " term " << k;
    }
}

TEST(Grid_samples, ThreadCountDoesNotChangeTheSamples) {

    std::vector<GridSample> one,
                            four;

    gridSamples(one,  24, 400, 4242, 1);
    gridSamples(four, 24, 400, 4242, 4);

    ASSERT_FALSE(one.empty());
    ASSERT_EQ(one.size(), four.size());

    //  Both the sample order and the holdout assignment must survive a change of workers.

    for (size_t i = 0; i < one.size(); ++i) {
        ASSERT_EQ(one[i].terms, four[i].terms) << i;
        ASSERT_EQ(one[i].score, four[i].score) << i;
        ASSERT_EQ(one[i].seen, four[i].seen) << i;
        ASSERT_EQ(one[i].withheld, four[i].withheld) << i;

        for (auto k = 0; k < one[i].terms; ++k)
            ASSERT_EQ(one[i].term[k], four[i].term[k]) << i << " term " << k;
    }
}

TEST(Grid_samples, HoldoutKeepsWholeGamesTogether) {

    std::vector<GridSample> first,
                            five,
                            six;

    //  Extending a seeded opening run keeps the earlier games unchanged. These prefixes
    //  locate the complete first and sixth games without assuming how many positions survive.

    gridSamples(first, 1, 400, 4242, 1);
    gridSamples(five,  5, 400, 4242, 1);
    gridSamples(six,   6, 400, 4242, 4);

    ASSERT_FALSE(first.empty());
    ASSERT_GT(five.size(), first.size());
    ASSERT_GT(six.size(), five.size());

    for (size_t i = 0; i < six.size(); ++i)
        EXPECT_EQ(i < first.size() || i >= five.size(), six[i].withheld) << i;

    std::ostringstream output;
    auto previous = std::cout.rdbuf(output.rdbuf());
    auto result = grid(6, 400, 4242, 4, 0.0);
    std::cout.rdbuf(previous);

    std::remove(gridSheet(6, 400, 4242).c_str());

    EXPECT_EQ(0, result);

    auto held = first.size() + six.size() - five.size();
    auto expected = "positions " + std::to_string(six.size()) + ", withheld " + std::to_string(held) + "\n";

    EXPECT_NE(std::string::npos, output.str().find(expected)) << output.str();
}

TEST(Grid_samples, RejectsAnEmptyTrainingPartition) {

    std::ostringstream output;
    auto previous = std::cout.rdbuf(output.rdbuf());
    auto result = grid(1, 400, 4242, 1, 0.0);
    std::cout.rdbuf(previous);

    std::remove(gridSheet(1, 400, 4242).c_str());

    EXPECT_EQ(1, result);
    EXPECT_NE(std::string::npos, output.str().find("both training and holdout sets"));
    EXPECT_EQ(std::string::npos, output.str().find("refitted on every position"));
}

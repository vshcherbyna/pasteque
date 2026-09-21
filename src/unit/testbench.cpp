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

#include <iostream>
#include <sstream>
#include <string>

#include "../bench.h"
#include "../board.h"
#include "../moves.h"

pasteque_namespace_begin
namespace unit
{

static std::string run(int depth)
{
    std::ostringstream captured;

    auto previous = std::cout.rdbuf(captured.rdbuf());

    bench(depth);

    std::cout.rdbuf(previous);

    return captured.str();
}

static size_t tally(const std::string & text, const std::string & needle)
{
    size_t total = 0;

    for (auto at = text.find(needle); at != std::string::npos; at = text.find(needle, at + needle.size()))
        ++total;

    return total;
}

static std::string lastLine(const std::string & text)
{
    auto end = text.find_last_not_of("\r\n");

    if (end == std::string::npos)
        return "";

    auto start = text.find_last_of('\n', end);

    return text.substr(start == std::string::npos ? 0 : start + 1, end - (start == std::string::npos ? 0 : start));
}

//
//  A bench position that no longer parses would abort the run on the rig rather than
//  here, and a position with no legal move left would quietly contribute nothing
//

TEST(Bench_positions, Positive)
{
    EXPECT_GT(benchCount(), 0);

    for (auto i = 0; i < benchCount(); ++i)
    {
        Board board;
        Moves legal;

        ASSERT_TRUE(board.setFen(benchPosition(i))) << i << "  " << benchPosition(i);

        //  the list is stored the way fen() writes it, so a round trip is exact

        EXPECT_EQ(board.fen(), std::string(benchPosition(i))) << i;

        legal.generateLegal(board);

        EXPECT_GT(legal.size(), 0) << i << "  " << benchPosition(i);
    }
}

//
//  OpenBench reads one line of this output and gives up if it cannot find it. Everything
//  above that line therefore has to stay clear of the two words it looks for
//

TEST(Bench_output, Positive)
{
    auto text = run(1);

    EXPECT_EQ(tally(text, "nodes"), 1u);
    EXPECT_EQ(tally(text, "nps"), 1u);

    auto summary = lastLine(text);

    EXPECT_NE(summary.find(" nodes "), std::string::npos) << summary;
    EXPECT_NE(summary.find(" nps"), std::string::npos) << summary;

    //  a count and a rate, in that order, with the count first on the line

    unsigned long long nodes = 0;
    std::string word;

    std::istringstream reader(summary);

    EXPECT_TRUE(static_cast<bool>(reader >> nodes >> word));
    EXPECT_EQ(word, "nodes");
    EXPECT_GT(nodes, 0u);
}

//
//  Searching deeper has to cost more, or the depth argument is not reaching the search
//

TEST(Bench_depth, Positive)
{
    auto shallow = run(1);
    auto deeper  = run(3);

    unsigned long long few = 0, many = 0;

    std::istringstream(lastLine(shallow)) >> few;
    std::istringstream(lastLine(deeper))  >> many;

    EXPECT_GT(many, few);
}

}
pasteque_namespace_end

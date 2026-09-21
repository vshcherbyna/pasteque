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

#include <string>

#include "../search.h"
#include "../judge.h"
#include "../moves.h"

pasteque_namespace_begin
namespace unit
{

struct Result
{
    std::string move;
    int         score;
};

static Result search(const char * fen, int depth)
{
    Board  board;
    Search searcher;

    EXPECT_TRUE(board.setFen(fen)) << fen;

    auto move = searcher.bestMove(board, depth);

    //  the search must give the board back exactly as it took it

    EXPECT_EQ(board.fen(), std::string(fen)) << fen;

    return { move.toString(), searcher.getScore() };
}

TEST(Search, FindsMateInOne)
{
    auto rook = search("6k1/5ppp/8/8/8/8/8/R3K2R w KQ - 0 1", 3);

    EXPECT_EQ(rook.move, "a1a8");
    EXPECT_EQ(rook.score, int(MATE_SCORE) - 1);

    auto backRank = search("6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1", 3);

    EXPECT_EQ(backRank.move, "a1a8");
    EXPECT_EQ(backRank.score, int(MATE_SCORE) - 1);
}

TEST(Search, FindsMateInTwo)
{
    //  two rooks against a bare king, the ladder takes two moves

    auto ladder = search("7k/8/8/8/8/8/8/RR2K3 w - - 0 1", 4);

    EXPECT_EQ(ladder.score, int(MATE_SCORE) - 3);
}

//
//  The evaluation terms are drawn at random, so no test here may assume the engine wants
//  material or any other chess. These are the properties that hold whatever the tables say
//

static const char * SEARCHED[] = {
    START_POSITION,
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"
};

TEST(Search, ReturnsALegalMove)
{
    for (auto fen : SEARCHED)
    {
        Board board;
        Moves legal;

        EXPECT_TRUE(board.setFen(fen)) << fen;

        legal.generateLegal(board);

        auto found = search(fen, 4);
        auto seen  = false;

        for (auto i = 0; i < legal.size(); ++i)
            if (legal[i].toString() == found.move)
                seen = true;

        EXPECT_TRUE(seen) << fen << "  ->  " << found.move;
    }
}

TEST(Search, IsRepeatable)
{
    for (auto fen : SEARCHED)
    {
        auto first  = search(fen, 4);
        auto second = search(fen, 4);

        EXPECT_EQ(first.move, second.move) << fen;
        EXPECT_EQ(first.score, second.score) << fen;
    }
}

TEST(Search, StaysOutOfTheMateWindow)
{
    //  a quiet position must never score where the search would read a mate

    for (auto fen : SEARCHED)
    {
        auto found = search(fen, 4);
        auto size  = (found.score < 0) ? -found.score : found.score;

        EXPECT_LT(size, int(MATE_SCORE) - int(PLY_LIMIT)) << fen;
    }
}

TEST(Search, ScoresTerminalPositions)
{
    //  no legal move at the root, so no move comes back

    auto mated = search("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 0 1", 4);

    EXPECT_EQ(mated.move, "a1a1");

    auto stalemated = search("7k/8/8/8/8/8/2q5/K7 w - - 0 1", 4);

    EXPECT_EQ(stalemated.move, "a1a1");
}

TEST(Search, DeepensWithoutLosingTheMove)
{
    //  the same position at rising depth keeps returning a move, and the node count grows
    //  rather than the search quietly doing nothing

    unsigned long long previous = 0;

    for (auto depth = 1; depth <= 4; ++depth)
    {
        Board  board;
        Search searcher;

        EXPECT_TRUE(board.setFen("rnbqkbnr/ppp2ppp/8/3pp3/6Q1/4P3/PPPP1PPP/RNB1KBNR b KQkq - 0 3"));

        auto move = searcher.bestMove(board, depth);

        EXPECT_NE(move.toString(), "a1a1");
        EXPECT_GT(searcher.getNodes(), previous);

        previous = searcher.getNodes();
    }
}

}
pasteque_namespace_end

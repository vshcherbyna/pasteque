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

#include <gtest/gtest.h>

#include <string>

#include "../search.h"
#include "../judge.h"

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

TEST(Search, SeesMaterial)
{
    //  a queen hanging on g4 to the bishop on c8

    auto queen = search("rnbqkbnr/ppp2ppp/8/3pp3/6Q1/4P3/PPPP1PPP/RNB1KBNR b KQkq - 0 3", 4);

    EXPECT_EQ(queen.move, "c8g4");
    EXPECT_GT(queen.score, 800);

    //  a rook checking from an undefended square gets taken

    auto rook = search("4k3/8/8/8/8/8/4r3/4K2R w K - 0 1", 4);

    EXPECT_EQ(rook.move, "e1e2");
    EXPECT_GT(rook.score, 400);
}

TEST(Search, QuiescenceHoldsTheScore)
{
    //  a queen that can take a defended pawn must not be tempted. Without a quiescence
    //  search the shallow score would show the pawn won and miss the recapture

    auto grab = search("4k3/8/8/3p4/8/8/3K4/3Q4 w - - 0 1", 2);

    EXPECT_LT(grab.score, 1200);
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
    //  the same position at rising depth keeps finding a sane capture, and the node count
    //  grows rather than the search quietly doing nothing

    unsigned long long previous = 0;

    for (auto depth = 1; depth <= 4; ++depth)
    {
        Board  board;
        Search searcher;

        EXPECT_TRUE(board.setFen("rnbqkbnr/ppp2ppp/8/3pp3/6Q1/4P3/PPPP1PPP/RNB1KBNR b KQkq - 0 3"));

        auto move = searcher.bestMove(board, depth);

        EXPECT_EQ(move.toString(), "c8g4");
        EXPECT_GT(searcher.getNodes(), previous);

        previous = searcher.getNodes();
    }
}

}
pasteque_namespace_end

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

static Result search(const char * fen, int depth) {
    Board  board;
    Search searcher;

    EXPECT_TRUE(board.setFen(fen)) << fen;

    auto move = searcher.bestMove(board, depth);

    //  the search must give the board back exactly as it took it

    EXPECT_EQ(board.fen(), std::string(fen)) << fen;

    return { move.toString(), searcher.getScore() };
}

static bool play(Board & board, const char * notation) {
    Moves  moves;
    Rewind undo;

    moves.generateLegal(board);

    for (auto i = 0; i < moves.size(); ++i) {
        if (moves[i].toString() != notation)
            continue;

        board.doMove(moves[i], undo);

        return true;
    }

    return false;
}

TEST(Search, MateOutranksTheFiftyMoveRule) {
    static const char * MATING = "6k1/5ppp/8/8/8/8/8/R3K2R w KQ - 99 1";

    auto shallow = search(MATING, 1);

    EXPECT_EQ(shallow.move, "a1a8");
    EXPECT_EQ(shallow.score, int(MATE_SCORE) - 1);

    auto deeper = search(MATING, 3);

    EXPECT_EQ(deeper.move, "a1a8");
    EXPECT_EQ(deeper.score, int(MATE_SCORE) - 1);
}

TEST(Search, StopsAtNodeQuota)
{
    Board  board;
    Search searcher;

    Clock counted({ "go", "nodes", "5000" }, board.getSide());

    ASSERT_EQ(counted.getNodes(), 5000ULL);

    searcher.bestMove(board, counted);

    EXPECT_EQ(searcher.getNodes(), 5000ULL);
}

TEST(Search, DepthSearchIgnoresTheQuota)
{
    Board  board;
    Search searcher;

    searcher.bestMove(board, 4);

    EXPECT_NE(searcher.getNodes(), 5000ULL);
    EXPECT_NE(searcher.getNodes(), 0ULL);
}

TEST(Search, QuiescenceScoresRepetitionAsDraw) {
    static const char * SHUFFLE[] = { "g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1", "f6g8" };

    Board b;

    ASSERT_TRUE(b.setFen("4k1n1/8/8/8/8/8/8/R3K1N1 w - - 0 1"));

    for (auto notation : SHUFFLE)
        ASSERT_TRUE(play(b, notation)) << notation;

    ASSERT_TRUE(b.recurred(0));
    ASSERT_NE(Judge::evaluate(b), int(EVEN_SCORE));

    Search searcher;

    EXPECT_EQ(searcher.quiescence(b, -int(HUGE_SCORE), int(HUGE_SCORE), 0), Judge::evaluate(b));
    EXPECT_EQ(searcher.quiescence(b, -int(HUGE_SCORE), int(HUGE_SCORE), 1), int(EVEN_SCORE));
}

TEST(Search, ScoresRepetitionAsDraw) {
    static const char * SHUFFLE[] = { "g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1" };

    for (auto depth = 1; depth <= 3; ++depth)
    {
        Board played;

        ASSERT_TRUE(played.setFen("4k1n1/8/8/8/8/8/8/R3K1N1 w - - 0 1"));

        for (auto notation : SHUFFLE)
            ASSERT_TRUE(play(played, notation)) << notation;

        Board bare;

        ASSERT_TRUE(bare.setFen(played.fen()));

        Search knowing,
               blind;

        auto repeat = knowing.bestMove(played, depth);
        auto fresh  = blind.bestMove(bare, depth);

        //
        //  The same position without its history has to be worth less than a draw, or the
        //  fixture proves nothing. Random evaluation terms decide that, so a tuned set of
        //  tables may need a different position here
        //

        ASSERT_LT(blind.getScore(), int(EVEN_SCORE)) << "depth " << depth;

        EXPECT_EQ(repeat.toString(), "f6g8") << "depth " << depth;
        EXPECT_EQ(knowing.getScore(), int(EVEN_SCORE)) << "depth " << depth;
        EXPECT_NE(fresh.toString(), repeat.toString()) << "depth " << depth;
    }
}

TEST(Search, LeavesRepetitionHistoryIntact) {
    Board searched,
          untouched;

    static const char * SHUFFLE[] = { "g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1" };

    for (auto notation : SHUFFLE) {
        ASSERT_TRUE(play(searched, notation)) << notation;
        ASSERT_TRUE(play(untouched, notation)) << notation;
    }

    Search searcher;

    searcher.bestMove(searched, 3);

    ASSERT_TRUE(play(searched, "f6g8"));
    ASSERT_TRUE(play(untouched, "f6g8"));

    EXPECT_EQ(searched.fen(), untouched.fen());
    EXPECT_EQ(searched.getStamp(), untouched.getStamp());

    EXPECT_TRUE(untouched.recurred(0));
    EXPECT_TRUE(searched.recurred(0));
}

TEST(Search, FindsMateInOne) {
    auto rook = search("6k1/5ppp/8/8/8/8/8/R3K2R w KQ - 0 1", 3);

    EXPECT_EQ(rook.move, "a1a8");
    EXPECT_EQ(rook.score, int(MATE_SCORE) - 1);

    auto backRank = search("6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1", 3);

    EXPECT_EQ(backRank.move, "a1a8");
    EXPECT_EQ(backRank.score, int(MATE_SCORE) - 1);
}

TEST(Search, FindsMateInTwo) {
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

TEST(Search, ReturnsALegalMove) {
    for (auto fen : SEARCHED) {
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

TEST(Search, IsRepeatable) {
    for (auto fen : SEARCHED) {
        auto first  = search(fen, 4);
        auto second = search(fen, 4);

        EXPECT_EQ(first.move, second.move) << fen;
        EXPECT_EQ(first.score, second.score) << fen;
    }
}

TEST(Search, StaysOutOfTheMateWindow) {
    //  a quiet position must never score where the search would read a mate

    for (auto fen : SEARCHED) {
        auto found = search(fen, 4);
        auto size  = (found.score < 0) ? -found.score : found.score;

        EXPECT_LT(size, int(MATE_SCORE) - int(PLY_LIMIT)) << fen;
    }
}

TEST(Search, ScoresTerminalPositions) {
    //  no legal move at the root, so no move comes back

    auto mated = search("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 0 1", 4);

    EXPECT_EQ(mated.move, "a1a1");

    auto stalemated = search("7k/8/8/8/8/8/2q5/K7 w - - 0 1", 4);

    EXPECT_EQ(stalemated.move, "a1a1");
}

TEST(Search, DeepensWithoutLosingTheMove) {
    //  the same position at rising depth keeps returning a move, and the node count grows
    //  rather than the search quietly doing nothing

    unsigned long long previous = 0;

    for (auto depth = 1; depth <= 4; ++depth) {
        Board  board;
        Search searcher;

        EXPECT_TRUE(board.setFen("rnbqkbnr/ppp2ppp/8/3pp3/6Q1/4P3/PPPP1PPP/RNB1KBNR b KQkq - 0 3"));

        auto move = searcher.bestMove(board, depth);

        EXPECT_NE(move.toString(), "a1a1");
        EXPECT_GT(searcher.getNodes(), previous);

        previous = searcher.getNodes();
    }
}

TEST(Search_order, VictimThenAttacker) {
    Board  board;
    Search searcher;
    Moves  moves;

    ASSERT_TRUE(board.setFen("7k/8/8/2n1q3/3P4/8/8/4R1K1 w - - 0 1"));

    moves.generateLegal(board);
    searcher.order(moves);

    EXPECT_EQ(moves[0].toString(), "d4e5");
    EXPECT_EQ(moves[1].toString(), "e1e5");
    EXPECT_EQ(moves[2].toString(), "d4c5");
}

TEST(Search_order, BishopOutranksKnight) {

    //
    //  Taking the bishop with the queen has to beat taking the knight with a pawn. The fitted
    //  knight and bishop sit closer together than any other neighbouring pair, so this is where
    //  the ordering gives out first
    //

    Board  board;
    Search searcher;
    Moves  moves;

    ASSERT_TRUE(board.setFen("7k/8/8/2nb4/1P6/8/8/3Q2K1 w - - 0 1"));

    moves.generateLegal(board);
    searcher.order(moves);

    EXPECT_EQ(moves[0].toString(), "d1d5");
    EXPECT_EQ(moves[1].toString(), "b4c5");
}

TEST(Search_order, QueenOutranksRook) {

    //
    //  Taking the queen with a queen has to beat taking a rook with a pawn. The fitted rook
    //  and queen sit close together, so this is the pair the ordering can lose first
    //

    Board  board;
    Search searcher;
    Moves  moves;

    ASSERT_TRUE(board.setFen("7k/8/8/2rq4/1P6/8/8/3Q2K1 w - - 0 1"));

    moves.generateLegal(board);
    searcher.order(moves);

    EXPECT_EQ(moves[0].toString(), "d1d5");
    EXPECT_EQ(moves[1].toString(), "b4c5");
}

}
pasteque_namespace_end

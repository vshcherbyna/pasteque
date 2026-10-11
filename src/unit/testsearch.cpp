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

TEST(Search, StaysInsideEveryNodeQuota) {

    for (auto quota = 900ULL; quota <= 2400ULL; ++quota) {
        Board  board;
        Search searcher;

        std::vector<std::string> counting = { "go", "nodes", std::to_string(quota) };

        Clock counted(counting, board.getSide());

        searcher.bestMove(board, counted);

        EXPECT_EQ(searcher.getNodes(), quota) << "quota " << quota;
    }
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

static const char * CHECKING[] = {
    "4k3/8/8/8/8/8/4r3/4K3 w - - 0 1",
    "8/8/8/8/8/5k2/8/5K1R w - - 0 1",
    "6k1/8/8/8/8/8/8/R5KQ w - - 0 1",
    "2r3k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR w KQkq - 0 1"
};

TEST(Search, HandlesPositionsFullOfChecks) {
    for (auto fen : CHECKING) {
        Board board;
        Moves legal;

        ASSERT_TRUE(board.setFen(fen)) << fen;

        legal.generateLegal(board);

        ASSERT_NE(0, legal.size()) << fen;

        auto found = search(fen, 7);
        auto seen  = false;

        for (auto i = 0; i < legal.size(); ++i)
            if (legal[i].toString() == found.move)
                seen = true;

        EXPECT_TRUE(seen) << fen << "  ->  " << found.move;
    }
}

TEST(Search, PassingTheTurnLeavesTheRepetitionTrailWhereItWas) {
    Board nulled,
          untouched;

    static const char * SHUFFLE[] = { "g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1", "f6g8" };

    for (auto notation : SHUFFLE) {
        ASSERT_TRUE(play(nulled, notation)) << notation;
        ASSERT_TRUE(play(untouched, notation)) << notation;

        Rewind undo;

        nulled.doNull(undo);
        nulled.unmakeNull(undo);
    }

    EXPECT_EQ(untouched.fen(), nulled.fen());
    EXPECT_EQ(untouched.getStamp(), nulled.getStamp());

    ASSERT_TRUE(untouched.recurred(0));

    EXPECT_TRUE(nulled.recurred(0));
}

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
    searcher.order(moves, 0, Move());

    EXPECT_EQ(moves[0].toString(), "d4e5");
    EXPECT_EQ(moves[1].toString(), "e1e5");
    EXPECT_EQ(moves[2].toString(), "d4c5");
}

TEST(Search_order, AKillerOutranksTheRichestQuiet) {

    Board  board;
    Search searcher;
    Moves  moves;

    ASSERT_TRUE(board.setFen("4k3/8/8/8/8/8/8/R3K2R w - - 0 1"));

    moves.generateLegal(board);

    auto killer = moves[moves.size() - 1];
    auto richest = moves[0];

    for (auto i = 0; i < 4000; ++i)
        searcher.reward(richest, 64, 5);

    searcher.reward(killer, 1, 0);
    searcher.order(moves, 0, Move());

    EXPECT_EQ(moves[0].toString(), killer.toString());
}

TEST(Search, ASearchFillsTheOrderingTables) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w - - 4 4"));

    searcher.bestMove(board, 4);

    auto stored = 0,
         earned = 0;

    for (auto ply = 0; ply < static_cast<int>(PLY_LIMIT); ++ply)
        if (static_cast<int>(searcher.m_killers[ply][0]))
            ++stored;

    for (auto piece = 0; piece < 16; ++piece)
        for (auto square = 0; square < 64; ++square)
            if (searcher.m_merit[piece][square])
                ++earned;

    EXPECT_GT(stored, 0);
    EXPECT_GT(earned, 0);
}

TEST(Search_order, MeritOutlivesThePlyThatEarnedIt) {

    Board  board;
    Search searcher;
    Moves  moves;

    ASSERT_TRUE(board.setFen("4k3/8/8/8/8/8/8/R3K2R w - - 0 1"));

    moves.generateLegal(board);

    auto chosen = moves[moves.size() - 1];

    searcher.reward(chosen, 4, 3);
    searcher.order(moves, 0, Move());

    EXPECT_EQ(moves[0].toString(), chosen.toString());
}

TEST(Search_order, ACaptureOutranksASaturatedQuiet) {

    Board  board;
    Search searcher;
    Moves  moves;

    ASSERT_TRUE(board.setFen("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1"));

    moves.generateLegal(board);

    Move quiet;

    for (auto i = 0; i < moves.size(); ++i)
        if (moves[i].getCapture() == EMPTY && !moves[i].getPromotion())
            quiet = moves[i];

    ASSERT_NE(0, static_cast<int>(quiet));

    for (auto i = 0; i < 4000; ++i)
        searcher.reward(quiet, 64, 7);

    searcher.order(moves, 0, Move());

    EXPECT_NE(EMPTY, moves[0].getCapture());
}

TEST(Search_hash, AnEntryBelongsToItsHalfmoveClock) {

    Search searcher;

    int  found = 0;
    Move favoured;

    stamp key = 0x1324354657687980ULL;

    searcher.store(key, 96, 1, 0, 40, HASH_EXACT, Move(21u));

    EXPECT_TRUE(searcher.probe(key, 96, 1, 0, -300, 300, found, favoured));
    EXPECT_FALSE(searcher.probe(key, 97, 1, 0, -300, 300, found, favoured));
    EXPECT_FALSE(searcher.probe(key, 98, 1, 0, -300, 300, found, favoured));
    EXPECT_FALSE(searcher.probe(key, 95, 1, 0, -300, 300, found, favoured));
}

TEST(Search_hash, BelowTheHorizonTheClockIsIgnoredOnPurpose) {

    Search searcher;

    int  found = 0;
    Move favoured;

    stamp key = 0x2435465768798a9bULL;

    searcher.store(key, 0, 1, 0, 40, HASH_EXACT, Move(22u));

    EXPECT_TRUE(searcher.probe(key, 0, 1, 0, -300, 300, found, favoured));
    EXPECT_TRUE(searcher.probe(key, static_cast<int>(HASH_HORIZON) - 1, 1, 0, -300, 300, found, favoured));
    EXPECT_FALSE(searcher.probe(key, static_cast<int>(HASH_HORIZON), 1, 0, -300, 300, found, favoured));
}

TEST(Search_hash, LoweringHashReleasesTheMemory) {

    Search searcher;

    searcher.setHash(64);

    auto roomy = searcher.m_hash.capacity();

    searcher.setHash(1);

    EXPECT_LT(searcher.m_hash.capacity(), roomy);
    EXPECT_EQ(searcher.m_hash.capacity(), searcher.getSlots());
}

TEST(Search_hash, SetHashRoundsDownToAPowerOfTwo) {

    Search searcher;

    EXPECT_EQ(static_cast<size_t>(262144), searcher.getSlots());

    searcher.setHash(1);
    EXPECT_EQ(static_cast<size_t>(65536), searcher.getSlots());

    searcher.setHash(64);
    EXPECT_EQ(static_cast<size_t>(4194304), searcher.getSlots());

    searcher.setHash(1000);
    EXPECT_EQ(static_cast<size_t>(33554432), searcher.getSlots());
}

TEST(Search_hash, SetHashClampsWhatItIsGiven) {

    Search searcher;

    searcher.setHash(0);

    auto least = searcher.getSlots();

    searcher.setHash(-99);
    EXPECT_EQ(least, searcher.getSlots());

    searcher.setHash(HASH_MOST);

    auto most = searcher.getSlots();

    searcher.setHash(999999);
    EXPECT_EQ(most, searcher.getSlots());

    EXPECT_GT(most, least);
}

TEST(Search_hash, ResizingEmptiesTheTable) {

    Search searcher;

    int  found = 0;
    Move favoured;

    stamp key = 0x7766554433221100ULL;

    searcher.store(key, 0, 6, 0, 64, HASH_EXACT, Move(12u));

    ASSERT_TRUE(searcher.probe(key, 0, 6, 0, -300, 300, found, favoured));

    searcher.setHash(16);

    EXPECT_FALSE(searcher.probe(key, 0, 6, 0, -300, 300, found, favoured));
}

TEST(Search_hash, AnEmptySlotIsNotAHit) {

    Search searcher;

    int  found = 0;
    Move favoured;

    EXPECT_FALSE(searcher.probe(0x12345678ULL, 0, 1, 1, -100, 100, found, favoured));
    EXPECT_EQ(0, static_cast<int>(favoured));
}

TEST(Search_hash, KeepsAndFindsAnExactScore) {

    Search searcher;

    int  found = 0;
    Move favoured;

    searcher.store(0x1234567890abcdefULL, 0, 6, 3, 142, HASH_EXACT, Move(4321u));

    ASSERT_TRUE(searcher.probe(0x1234567890abcdefULL, 0, 6, 3, -100, 100, found, favoured));

    EXPECT_EQ(142, found);
    EXPECT_EQ(4321, static_cast<int>(favoured));
}

TEST(Search_hash, AMateScoreComesBackAtTheSamePly) {

    Search searcher;

    int  found = 0;
    Move favoured;

    auto mate = static_cast<int>(MATE_SCORE) - 9;

    searcher.store(0xfeedfacecafebeefULL, 0, 4, 9, mate, HASH_EXACT, Move(1u));

    ASSERT_TRUE(searcher.probe(0xfeedfacecafebeefULL, 0, 4, 9, -30000, 30000, found, favoured));

    EXPECT_EQ(mate, found);
}

TEST(Search_hash, BoundsOnlyAnswerWhereTheyApply) {

    Search searcher;

    int  found = 0;
    Move favoured;

    searcher.store(0x1111222233334444ULL, 0, 5, 0, 50, HASH_UPPER, Move(7u));

    EXPECT_TRUE(searcher.probe(0x1111222233334444ULL, 0, 5, 0, 60, 120, found, favoured));
    EXPECT_FALSE(searcher.probe(0x1111222233334444ULL, 0, 5, 0, 10, 120, found, favoured));

    searcher.store(0x5555666677778888ULL, 0, 5, 0, 50, HASH_LOWER, Move(7u));

    EXPECT_TRUE(searcher.probe(0x5555666677778888ULL, 0, 5, 0, -120, 40, found, favoured));
    EXPECT_FALSE(searcher.probe(0x5555666677778888ULL, 0, 5, 0, -120, 90, found, favoured));
}

TEST(Search_hash, AShallowEntryStillOffersItsMove) {

    Search searcher;

    int  found = 0;
    Move favoured;

    searcher.store(0x99aabbccddeeff00ULL, 0, 2, 0, 33, HASH_EXACT, Move(555u));

    EXPECT_FALSE(searcher.probe(0x99aabbccddeeff00ULL, 0, 8, 0, -100, 100, found, favoured));
    EXPECT_EQ(555, static_cast<int>(favoured));
}

TEST(Search_hash, ANeighbourInTheSameSlotIsNotAHit) {

    Search searcher;

    int  found = 0;
    Move favoured;

    stamp key = 0xabcd000012345678ULL;

    searcher.store(key, 0, 6, 0, 77, HASH_EXACT, Move(9u));

    EXPECT_TRUE(searcher.probe(key, 0, 6, 0, -100, 100, found, favoured));

    EXPECT_FALSE(searcher.probe(key ^ (1ULL << 40), 0, 6, 0, -100, 100, found, favoured));
}

TEST(Search_order, TheHashMoveLeadsEvenPastACapture) {

    Board  board;
    Search searcher;
    Moves  moves;

    ASSERT_TRUE(board.setFen("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1"));

    moves.generateLegal(board);

    Move quiet;

    for (auto i = 0; i < moves.size(); ++i)
        if (moves[i].getCapture() == EMPTY && !moves[i].getPromotion())
            quiet = moves[i];

    ASSERT_NE(0, static_cast<int>(quiet));

    searcher.order(moves, 0, quiet);

    EXPECT_EQ(moves[0].toString(), quiet.toString());
    EXPECT_EQ(EMPTY, moves[0].getCapture());
}

TEST(Search, TheTableDoesNotCarryIntoTheNextSearch) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w - - 4 4"));

    searcher.bestMove(board, 5);

    auto first = searcher.getNodes();

    searcher.bestMove(board, 5);

    auto again = searcher.getNodes();

    EXPECT_EQ(first, again);
}

TEST(Search, TheFiftyMoveRuleOutranksAHashScore) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("8/2K5/6Q1/1k6/8/8/8/8 w - - 94 1"));

    for (auto depth = 6; depth <= 8; ++depth) {
        searcher.bestMove(board, depth);

        EXPECT_EQ(static_cast<int>(EVEN_SCORE), searcher.getScore()) << "depth " << depth;
    }
}

TEST(Search, CheckmateOutranksTheFiftyMoveRule) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("7k/8/6QK/8/8/8/8/8 w - - 99 1"));

    auto best = searcher.bestMove(board, 3);

    EXPECT_EQ("g6g7", best.toString());
    EXPECT_GT(searcher.getScore(), static_cast<int>(MATE_SCORE) - static_cast<int>(PLY_LIMIT));
}

TEST(Search_hash, AnEntryIsJudgedByItsOwnDepthNotTheRequest) {

    Search searcher;

    int  found = 0;
    Move favoured;

    stamp key = 0x0a1b2c3d4e5f6071ULL;

    searcher.store(key, 0, 5, 0, 50, HASH_EXACT, Move(8u));

    EXPECT_TRUE(searcher.probe(key, 0, 1, 0, -300, 300, found, favoured));
    EXPECT_FALSE(searcher.probe(key, 96, 1, 0, -300, 300, found, favoured));
}

TEST(Search_hash, ADeeperEntryIsRefusedNearTheFiftyMoveRule) {

    Search searcher;

    int  found = 0;
    Move favoured;

    stamp key = 0x2468ace013579bdfULL;

    searcher.store(key, 0, 3, 0, static_cast<int>(MATE_SCORE) - 3, HASH_EXACT, Move(3u));

    EXPECT_TRUE(searcher.probe(key, 0, 1, 0, -30000, 30000, found, favoured));
    EXPECT_FALSE(searcher.probe(key, 98, 1, 0, -30000, 30000, found, favoured));

    EXPECT_EQ(3, static_cast<int>(favoured));
}

TEST(Search_hash, AMateIsNotKeptWhenTheDrawArrivesFirst) {

    Search searcher;

    int  found = 0;
    Move favoured;

    stamp key = 0x1357ace2468bdf09ULL;

    searcher.store(key, 94, 2, 0, static_cast<int>(MATE_SCORE) - 9, HASH_EXACT, Move(5u));

    EXPECT_FALSE(searcher.probe(key, 94, 1, 0, -30000, 30000, found, favoured));
    EXPECT_EQ(0, static_cast<int>(favoured));
}

TEST(Search, ACachedScoreDoesNotOutrunTheDraw) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("8/r3b3/8/8/2k5/5Q2/3K4/8 b - - 94 1"));

    for (auto depth = 5; depth <= 6; ++depth) {
        searcher.bestMove(board, depth);

        EXPECT_EQ(static_cast<int>(EVEN_SCORE), searcher.getScore()) << "depth " << depth;
    }
}

TEST(Search, AMateBeyondTheDrawIsNotClaimed) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("8/8/1k6/8/1K6/8/8/2Q5 w - - 92 1"));

    for (auto depth = 6; depth <= 9; ++depth) {
        searcher.bestMove(board, depth);

        auto score = searcher.getScore();

        EXPECT_LT(score, static_cast<int>(MATE_SCORE) - static_cast<int>(PLY_LIMIT)) << "depth " << depth;

        if (depth >= 8) {
            EXPECT_EQ(static_cast<int>(EVEN_SCORE), score) << "depth " << depth;
        }
    }
}

TEST(Search_hash, ADeeperEntrySurvivesAShallowOne) {

    Search searcher;

    int  found = 0;
    Move favoured;

    stamp key = 0x0f1e2d3c4b5a6978ULL;

    searcher.store(key, 0, 8, 0, 111, HASH_EXACT, Move(11u));
    searcher.store(key, 0, 2, 0, 222, HASH_EXACT, Move(22u));

    ASSERT_TRUE(searcher.probe(key, 0, 8, 0, -500, 500, found, favoured));

    EXPECT_EQ(111, found);
    EXPECT_EQ(11, static_cast<int>(favoured));
}

TEST(Search_hash, ASearchFillsTheTable) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w - - 4 4"));

    searcher.bestMove(board, 5);

    auto filled = 0;

    for (size_t i = 0; i < searcher.m_slots; ++i)
        if (searcher.m_hash[i].age == searcher.m_age)
            ++filled;

    EXPECT_GT(filled, 0);
}

TEST(Search_nullAllowed, APlainMidgameNodeIsAllowed) {

    Board  board;
    Search searcher;

    EXPECT_TRUE(searcher.nullAllowed(board, 8, 1, 50, false));
    EXPECT_TRUE(searcher.nullAllowed(board, static_cast<int>(NULL_DEPTH), 4, 50, false));
}

TEST(Search_nullAllowed, NotWhileInCheckAndNotAtTheRoot) {

    Board  board;
    Search searcher;

    EXPECT_FALSE(searcher.nullAllowed(board, 8, 1, 50, true));
    EXPECT_FALSE(searcher.nullAllowed(board, 8, 0, 50, false));
    EXPECT_FALSE(searcher.nullAllowed(board, 8, -1, 50, false));
}

TEST(Search_nullAllowed, NeverTwiceInARow) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(searcher.nullAllowed(board, 8, 3, 50, false));

    searcher.m_nulled[3] = true;

    EXPECT_FALSE(searcher.nullAllowed(board, 8, 3, 50, false));
    EXPECT_TRUE(searcher.nullAllowed(board, 8, 4, 50, false));

    searcher.m_nulled[3] = false;
}

TEST(Search_nullAllowed, NotTooShallowAndNotAgainstAMateWindow) {

    Board  board;
    Search searcher;

    for (auto depth = 0; depth < static_cast<int>(NULL_DEPTH); ++depth)
        EXPECT_FALSE(searcher.nullAllowed(board, depth, 2, 50, false)) << "depth " << depth;

    EXPECT_FALSE(searcher.nullAllowed(board, 8, 2, static_cast<int>(MATE_SCORE) - static_cast<int>(PLY_LIMIT), false));
    EXPECT_FALSE(searcher.nullAllowed(board, 8, 2, static_cast<int>(MATE_SCORE), false));
    EXPECT_TRUE(searcher.nullAllowed(board, 8, 2, static_cast<int>(MATE_SCORE) - static_cast<int>(PLY_LIMIT) - 1, false));
}

TEST(Search_nullAllowed, NotAgainstAMateWindowOnEitherSide) {

    Board  board;
    Search searcher;

    auto reach = static_cast<int>(MATE_SCORE) - static_cast<int>(PLY_LIMIT);

    EXPECT_FALSE(searcher.nullAllowed(board, 8, 2, reach, false));
    EXPECT_FALSE(searcher.nullAllowed(board, 8, 2, static_cast<int>(MATE_SCORE), false));
    EXPECT_FALSE(searcher.nullAllowed(board, 8, 2, -reach, false));
    EXPECT_FALSE(searcher.nullAllowed(board, 8, 2, -static_cast<int>(MATE_SCORE), false));

    EXPECT_TRUE(searcher.nullAllowed(board, 8, 2, reach - 1, false));
    EXPECT_TRUE(searcher.nullAllowed(board, 8, 2, -reach + 1, false));
}

TEST(Search, APassCannotManufactureAFiftyMoveDraw) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("7k/6pr/5N1p/5N2/8/8/8/R3K3 b - - 99 1"));

    Moves moves;
    moves.generateLegal(board);

    ASSERT_EQ(4, moves.size());

    for (auto i = 0; i < moves.size(); ++i)
        ASSERT_EQ(static_cast<int>(PAWN), moves[i].getPiece() & 7) << moves[i].toString();

    EXPECT_LT(searcher.alphaBeta(board, -1, 0, 4, 1), 0);
}

TEST(Search, APassCannotCutOffAnUnprovenMate) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("7k/6pr/5N1p/5N2/8/8/8/R3K3 b - - 0 1"));

    auto alpha = -(static_cast<int>(MATE_SCORE) - 9);
    auto beta  = -(static_cast<int>(MATE_SCORE) - 10);

    EXPECT_LT(searcher.alphaBeta(board, alpha, beta, 3, 1), beta);
}

TEST(Search_nullAllowed, NotWhenTheFiftyMoveBoundaryIsInReach) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("8/8/8/4k3/8/4K3/8/7R w - - 95 1"));

    EXPECT_FALSE(searcher.nullAllowed(board, 5, 2, 50, false));
    EXPECT_TRUE(searcher.nullAllowed(board, 4, 2, 50, false));

    ASSERT_TRUE(board.setFen("8/8/8/4k3/8/4K3/8/7R w - - 0 1"));

    EXPECT_TRUE(searcher.nullAllowed(board, 64, 2, 50, false));
}

TEST(Search, APassCannotSkipAnUnavoidableFiftyMoveDraw) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("8/2K5/6Q1/1k6/8/8/8/8 w - - 99 1"));

    Moves moves;
    moves.generateLegal(board);

    ASSERT_EQ(29, moves.size());

    for (auto i = 0; i < moves.size(); ++i) {

        Rewind undo;

        board.doMove(moves[i], undo);

        ASSERT_EQ(100u, board.getFifty()) << moves[i].toString();

        Moves replies;
        replies.generateLegal(board);

        ASSERT_NE(0, replies.size()) << moves[i].toString();

        board.unmakeMove(moves[i], undo);
    }

    EXPECT_EQ(static_cast<int>(EVEN_SCORE), searcher.alphaBeta(board, 99, 100, 3, 1));
}

TEST(Search, ADeeperPassCannotManufactureAFiftyMoveDraw) {

    Board  board;
    Search searcher;
    Search without;

    ASSERT_TRUE(board.setFen("7k/5Kpr/7p/8/8/N7/Q7/8 b - - 99 1"));

    Moves moves;
    moves.generateLegal(board);

    ASSERT_EQ(3, moves.size());

    for (auto i = 0; i < moves.size(); ++i)
        ASSERT_EQ(static_cast<int>(PAWN), moves[i].getPiece() & 7) << moves[i].toString();

    for (auto ply = 0; ply < static_cast<int>(PLY_LIMIT); ++ply)
        without.m_nulled[ply] = true;

    for (auto depth = 4; depth <= 6; ++depth) {

        auto expected = without.alphaBeta(board, -1, 0, depth, 1);

        ASSERT_LT(expected, 0) << "depth " << depth;
        EXPECT_LT(searcher.alphaBeta(board, -1, 0, depth, 1), 0) << "depth " << depth << ", " << expected << " without passes";
    }
}

TEST(Search_nullAllowed, NotWhenThereIsNothingButPawnsToMove) {

    Board  board;
    Search searcher;

    ASSERT_TRUE(board.setFen("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1"));

    EXPECT_FALSE(searcher.nullAllowed(board, 8, 2, 50, false));

    ASSERT_TRUE(board.setFen("4k3/pppppppp/8/8/8/8/PPPPPPPP/4KN2 w - - 0 1"));

    EXPECT_TRUE(searcher.nullAllowed(board, 8, 2, 50, false));

    ASSERT_TRUE(board.setFen("4kn2/pppppppp/8/8/8/8/PPPPPPPP/4K3 b - - 0 1"));

    EXPECT_TRUE(searcher.nullAllowed(board, 8, 2, 50, false));
}

TEST(Search_nullAllowed, NotWhereTheFlagWouldRunOffTheEnd) {

    Board  board;
    Search searcher;

    EXPECT_FALSE(searcher.nullAllowed(board, 8, static_cast<int>(PLY_LIMIT) - 1, 50, false));
    EXPECT_FALSE(searcher.nullAllowed(board, 8, static_cast<int>(PLY_LIMIT), 50, false));
    EXPECT_TRUE(searcher.nullAllowed(board, 8, static_cast<int>(PLY_LIMIT) - 2, 50, false));
}

TEST(Search_verify, AScoreThatDoesNotBeatAlphaIsNeverSearchedAgain) {

    Search searcher;

    for (auto reduction = 0; reduction <= 2; ++reduction) {
        EXPECT_FALSE(searcher.verify(10, 10, 50, reduction)) << "reduction " << reduction;
        EXPECT_FALSE(searcher.verify(-40, 10, 50, reduction)) << "reduction " << reduction;
    }
}

TEST(Search_verify, AWholeSearchIsRedoneOnlyInsideTheWindow) {

    Search searcher;

    EXPECT_TRUE(searcher.verify(30, 10, 50, 0));
    EXPECT_FALSE(searcher.verify(50, 10, 50, 0));
    EXPECT_FALSE(searcher.verify(90, 10, 50, 0));
}

TEST(Search_verify, AReducedSearchIsRedoneEvenAboveBeta) {

    Search searcher;

    EXPECT_TRUE(searcher.verify(30, 10, 50, 1));
    EXPECT_TRUE(searcher.verify(50, 10, 50, 1));
    EXPECT_TRUE(searcher.verify(90, 10, 50, 1));
    EXPECT_TRUE(searcher.verify(90, 10, 50, 2));
}

TEST(Search_verify, TheWindowAndTheReductionAreBothConsulted) {

    Search searcher;

    for (auto score = -20; score <= 90; ++score)
        for (auto reduction = 0; reduction <= 2; ++reduction) {
            auto wanted = score > 10 && (reduction != 0 || score < 50);

            EXPECT_EQ(wanted, searcher.verify(score, 10, 50, reduction))
                << "score " << score << " reduction " << reduction;
        }
}

TEST(Search_reduce, NothingIsReducedWhileInCheck) {

    Search searcher;

    Move quiet(12, 28, 1);

    EXPECT_EQ(0, searcher.reduce(quiet, 8, 8, true, false));
    EXPECT_EQ(0, searcher.reduce(quiet, 20, 30, true, false));
}

TEST(Search_reduce, NothingIsReducedForAMoveThatGivesCheck) {

    Search searcher;

    Move quiet(12, 28, 1);

    EXPECT_EQ(0, searcher.reduce(quiet, 8, 8, false, true));
    EXPECT_EQ(0, searcher.reduce(quiet, 20, 30, false, true));
}

TEST(Search_reduce, NothingIsReducedForCapturesOrPromotions) {

    Search searcher;

    Move capture(12, 28, 1, 5);
    Move promotion(52, 60, 1, 0, 4);
    Move both(52, 61, 1, 5, 4);

    EXPECT_EQ(0, searcher.reduce(capture, 8, 8, false, false));
    EXPECT_EQ(0, searcher.reduce(promotion, 8, 8, false, false));
    EXPECT_EQ(0, searcher.reduce(both, 8, 8, false, false));
}

TEST(Search_reduce, TheFirstMovesAndTheShallowSearchesAreWhole) {

    Search searcher;

    Move quiet(12, 28, 1);

    for (auto played = 0; played < static_cast<int>(REDUCE_MOVES); ++played)
        EXPECT_EQ(0, searcher.reduce(quiet, 8, played, false, false)) << "played " << played;

    for (auto depth = 1; depth < static_cast<int>(REDUCE_DEPTH); ++depth)
        EXPECT_EQ(0, searcher.reduce(quiet, depth, 8, false, false)) << "depth " << depth;

    EXPECT_LT(0, searcher.reduce(quiet, static_cast<int>(REDUCE_DEPTH), static_cast<int>(REDUCE_MOVES), false, false));
}

TEST(Search_reduce, ALateQuietMoveLosesAPlyAndADeeperOneLosesTwo) {

    Search searcher;

    Move quiet(12, 28, 1);

    EXPECT_EQ(1, searcher.reduce(quiet, 3, 3, false, false));
    EXPECT_EQ(1, searcher.reduce(quiet, 5, 5, false, false));
    EXPECT_EQ(1, searcher.reduce(quiet, 20, 5, false, false));
    EXPECT_EQ(1, searcher.reduce(quiet, 5, 20, false, false));
    EXPECT_EQ(2, searcher.reduce(quiet, 6, 6, false, false));
    EXPECT_EQ(2, searcher.reduce(quiet, 20, 30, false, false));
}

TEST(Search_reduce, AReducedSearchKeepsAtLeastOneRealPly) {

    Search searcher;

    Move quiet(12, 28, 1);

    for (auto depth = 1; depth <= static_cast<int>(PLY_LIMIT); ++depth)
        for (auto played = 0; played < 48; ++played) {
            auto reduction = searcher.reduce(quiet, depth, played, false, false);

            EXPECT_LE(0, reduction) << "depth " << depth << " played " << played;

            if (reduction) {
                EXPECT_LE(1, depth - 1 - reduction) << "depth " << depth << " played " << played;
            }
        }
}

TEST(Search, ReductionsDoNotLoseAMate) {

    for (auto depth = 4; depth <= 10; depth += 2) {
        auto ladder = search("7k/8/8/8/8/8/8/RR2K3 w - - 0 1", depth);

        EXPECT_EQ(static_cast<int>(MATE_SCORE) - 3, ladder.score) << "depth " << depth;
    }
}

TEST(Search, OrderingDoesNotLeakBetweenSearches) {

    Board first,
          second;

    ASSERT_TRUE(first.setFen("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w - - 4 4"));
    ASSERT_TRUE(second.setFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"));

    Search reused,
           fresh;

    reused.bestMove(first, 4);

    auto warmed = reused.bestMove(second, 4);
    auto after  = reused.getNodes();

    auto cold  = fresh.bestMove(second, 4);
    auto alone = fresh.getNodes();

    EXPECT_EQ(static_cast<int>(cold), static_cast<int>(warmed));
    EXPECT_EQ(alone, after);
}

TEST(Search_order, KnightOutranksPawn) {

    //
    //  The cheapest neighbouring pair. Taking the knight with the queen has to beat taking the
    //  pawn with a pawn, which is the widest margin of the four and so the least likely to give
    //  out - it is here because the pair that binds has moved on both refits so far
    //

    Board  board;
    Search searcher;
    Moves  moves;

    ASSERT_TRUE(board.setFen("7k/8/8/2pn4/1P6/8/8/3Q2K1 w - - 0 1"));

    moves.generateLegal(board);
    searcher.order(moves, 0, Move());

    EXPECT_EQ(moves[0].toString(), "d1d5");
    EXPECT_EQ(moves[1].toString(), "b4c5");
}

TEST(Search_order, RookOutranksBishop) {

    //
    //  Taking the rook with the queen has to beat taking the bishop with a pawn. The king stands
    //  on h1 rather than g1 because a bishop on c5 would otherwise be giving check, and a
    //  restricted move list would not be testing the ordering
    //

    Board  board;
    Search searcher;
    Moves  moves;

    ASSERT_TRUE(board.setFen("7k/8/8/2br4/1P6/8/8/3Q3K w - - 0 1"));

    moves.generateLegal(board);
    searcher.order(moves, 0, Move());

    EXPECT_EQ(moves[0].toString(), "d1d5");
    EXPECT_EQ(moves[1].toString(), "b4c5");
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
    searcher.order(moves, 0, Move());

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
    searcher.order(moves, 0, Move());

    EXPECT_EQ(moves[0].toString(), "d1d5");
    EXPECT_EQ(moves[1].toString(), "b4c5");
}

}
pasteque_namespace_end

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

#include <chrono>
#include <string>
#include <vector>

#include "../clock.h"
#include "../search.h"
#include "../judge.h"

pasteque_namespace_begin
namespace unit
{

static Clock read(const std::string & line, unsigned char side) {

    std::vector<std::string> tokens;
    std::string token;

    for (auto character : line + " ") {
        if (character == ' ') {
            if (!token.empty())
                tokens.push_back(token);

            token.clear();
        }
        else
            token += character;
    }

    return Clock(tokens, side);
}

TEST(Clock, Positive) {

    Clock idle;

    EXPECT_TRUE(idle.isEndless());
    EXPECT_EQ(idle.getDepth(), int(DEPTH_CEILING));

    //  a bare go names no limit, so it is answered quickly rather than never

    auto bare = read("go", WHITE);

    EXPECT_TRUE(bare.isEndless());
    EXPECT_EQ(bare.getDepth(), int(DEFAULT_DEPTH));

    //  stop cannot interrupt a search, so an endless one would never answer

    auto endless = read("go infinite", WHITE);

    EXPECT_TRUE(endless.isEndless());
    EXPECT_EQ(endless.getDepth(), int(DEFAULT_DEPTH));

    auto deep = read("go depth 8", WHITE);

    EXPECT_TRUE(deep.isEndless());
    EXPECT_EQ(deep.getDepth(), 8);
}

TEST(Clock_depth, Positive) {

    EXPECT_EQ(read("go depth 0", WHITE).getDepth(), 1);
    EXPECT_EQ(read("go depth -5", WHITE).getDepth(), 1);
    EXPECT_EQ(read("go depth 999", WHITE).getDepth(), int(DEPTH_CEILING));
    EXPECT_EQ(read("go depth", WHITE).getDepth(), int(DEFAULT_DEPTH));
}

TEST(Clock_movetime, Positive) {

    auto fixed = read("go movetime 1000", WHITE);

    EXPECT_FALSE(fixed.isEndless());
    EXPECT_EQ(fixed.getSoft(), 1000u - MOVE_OVERHEAD);
    EXPECT_EQ(fixed.getHard(), fixed.getSoft());

    //  an allowance smaller than the overhead still has to answer with something

    auto tiny = read("go movetime 5", WHITE);

    EXPECT_FALSE(tiny.isEndless());
    EXPECT_EQ(tiny.getSoft(), 1u);
}

TEST(Clock_suddenDeath, Positive) {

    auto plain = read("go wtime 60000 btime 60000", WHITE);

    EXPECT_FALSE(plain.isEndless());
    EXPECT_EQ(plain.getSoft(), 60000u / SUDDEN_DEATH_MOVES);
    EXPECT_EQ(plain.getHard(), plain.getSoft() * HARD_MULTIPLIER);

    //  three quarters of the increment is spendable on top of the share

    auto gaining = read("go wtime 60000 winc 1000", WHITE);

    EXPECT_EQ(gaining.getSoft(), 60000u / SUDDEN_DEATH_MOVES + 750u);
}

TEST(Clock_movesToGo, Positive) {

    auto cyclic = read("go wtime 60000 movestogo 10", WHITE);

    EXPECT_FALSE(cyclic.isEndless());
    EXPECT_EQ(cyclic.getSoft(), 6000u);
    EXPECT_EQ(cyclic.getHard(), 18000u);
}

TEST(Clock_side, Positive) {

    //  each side reads only its own clock and its own increment

    auto asWhite = read("go wtime 60000 btime 6000 winc 100 binc 900", WHITE);
    auto asBlack = read("go wtime 60000 btime 6000 winc 100 binc 900", BLACK);

    EXPECT_EQ(asWhite.getSoft(), 60000u / SUDDEN_DEATH_MOVES + 75u);
    EXPECT_EQ(asBlack.getSoft(), 6000u / SUDDEN_DEATH_MOVES + 675u);
}

TEST(Clock_lowTime, Positive) {

    //  nothing may come back as zero, and nothing may exceed what is left on the clock

    for (auto left = 1u; left <= 200u; ++left) {
        auto pressed = read("go wtime " + std::to_string(left), WHITE);

        EXPECT_GT(pressed.getSoft(), 0u);
        EXPECT_GT(pressed.getHard(), 0u);
        EXPECT_LE(pressed.getHard(), left > MOVE_OVERHEAD ? left - MOVE_OVERHEAD : 1u);
    }
}

//
//  A gui that reports an expired clock, as zero or as a negative number, must still get a
//  move back. Reading either as "no limit named" leaves the search endless, and nothing
//  can rescue it because stop is only read between searches
//

TEST(Clock_expired, Positive) {

    //  every one of these names a limit for both sides, so both must come out bounded

    static const char * LINES[] = {
        "go wtime 0 btime 0",
        "go wtime -1 btime -1",
        "go movetime 0",
        "go movetime -1",
        "go wtime -1 btime -1 winc -1 binc -1 movestogo -1"
    };

    for (auto line : LINES) {
        for (auto side : { WHITE, BLACK }) {
            auto pressed = read(line, static_cast<unsigned char>(side));

            EXPECT_FALSE(pressed.isEndless()) << line;
            EXPECT_GT(pressed.getSoft(), 0u) << line;
            EXPECT_GE(pressed.getHard(), pressed.getSoft()) << line;
        }
    }

    //  one side named, the other not: the named side is bounded by its clock

    auto named = read("go wtime 0 btime 1000", WHITE);

    EXPECT_FALSE(named.isEndless());
    EXPECT_GT(named.getSoft(), 0u);
}

//
//  Nothing at all is said about the side to move. There is no budget to compute, so this
//  is answered the way a bare go is, at a depth that returns rather than at the ceiling
//

TEST(Clock_silentAboutUs, Positive) {

    auto quiet = read("go wtime 1000 winc 100", BLACK);

    EXPECT_TRUE(quiet.isEndless());
    EXPECT_EQ(quiet.getDepth(), int(DEFAULT_DEPTH));
}

//
//  A negative increment used to be read as four billion milliseconds of bonus, which put
//  the whole remaining clock into one move
//

TEST(Clock_negativeIncrement, Positive) {

    auto spoiled = read("go wtime 60000 winc -5", WHITE);
    auto plain   = read("go wtime 60000", WHITE);

    EXPECT_EQ(spoiled.getSoft(), plain.getSoft());
    EXPECT_LT(spoiled.getSoft(), 60000u);
}

//
//  hard is soft times a constant, and that product must not wrap past hard on a clock
//  large enough to overflow the multiplication
//

TEST(Clock_hardNeverBelowSoft, Positive) {

    static const char * LINES[] = {
        "go wtime 1",
        "go wtime 1000",
        "go wtime 60000 winc 1000",
        "go wtime 2147483647 binc 2147483647 winc 2147483647",
        "go wtime 2000000000 movestogo 1",
        "go movetime 2147483647"
    };

    for (auto line : LINES) {
        auto huge = read(line, WHITE);

        EXPECT_GE(huge.getHard(), huge.getSoft()) << line;
        EXPECT_GT(huge.getSoft(), 0u) << line;
    }
}

TEST(Search_expiredClockStillAnswers, Positive) {

    Board  board;
    Search searcher;

    EXPECT_TRUE(board.setFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"));

    auto started = std::chrono::steady_clock::now();
    auto best    = searcher.bestMove(board, read("go wtime 0 btime 1000", WHITE));
    auto spent   = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - started).count();

    EXPECT_NE(static_cast<int>(best), 0);
    EXPECT_LT(spent, 2000);
}

TEST(Search_timed, Positive) {

    Board  board;
    Search searcher;

    EXPECT_TRUE(board.setFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"));

    auto clock   = read("go movetime 300", WHITE);
    auto started = std::chrono::steady_clock::now();
    auto best    = searcher.bestMove(board, clock);
    auto spent   = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - started).count();

    EXPECT_NE(static_cast<int>(best), 0);
    EXPECT_LT(spent, 2000);
    EXPECT_EQ(board.fen(), "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    //  a move cut off part way through is still a legal move from this position

    Moves legal;
    legal.generateLegal(board);

    auto found = false;

    for (auto i = 0; i < legal.size(); ++i)
        if (static_cast<int>(legal[i]) == static_cast<int>(best))
            found = true;

    EXPECT_TRUE(found);
}

static int  g_depthSeen;
static Move g_moveSeen;

static void note(int depth, int, unsigned long long, unsigned long long, Move best) {

    g_depthSeen = depth;
    g_moveSeen  = best;
}

TEST(Search_stopsShortOfTheCeiling, Positive) {

    Board  board;
    Search searcher;

    const char * kiwi = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

    EXPECT_TRUE(board.setFen(kiwi));

    searcher.setWatcher(note);

    g_depthSeen = 0;

    //
    //  Twenty plies here runs for minutes, so anything coming back promptly can only be
    //  the hard limit cutting the iteration short
    //

    auto clock   = read("go depth 20 movetime 200", WHITE);
    auto started = std::chrono::steady_clock::now();
    auto best    = searcher.bestMove(board, clock);
    auto spent   = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - started).count();

    EXPECT_LT(spent, 2000);
    EXPECT_NE(static_cast<int>(best), 0);

    EXPECT_GT(g_depthSeen, 0);
    EXPECT_LT(g_depthSeen, 20);

    //  every doMove on the way down was matched on the way back out

    EXPECT_EQ(board.fen(), std::string(kiwi));

    //  the move handed back is the one the last finished iteration settled on

    EXPECT_EQ(static_cast<int>(best), static_cast<int>(g_moveSeen));
}

TEST(Search_hardLimit, Positive) {

    Board  board;
    Search searcher;

    const char * kiwi = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

    EXPECT_TRUE(board.setFen(kiwi));

    searcher.setWatcher(note);

    g_depthSeen = 0;

    //
    //  A soft budget this large never stops an iteration from starting, so the only thing
    //  that can end the search is the hard limit firing part way through one. Deriving
    //  soft and hard from a clock always leaves soft the smaller of the two, which is why
    //  this reaches for the explicit form
    //

    //
    //  The ceiling is kept low enough that a hard limit which never fires still finishes,
    //  in about ten seconds, and fails on the elapsed time rather than hanging the suite
    //

    Clock tight(1000000, 5, 8);

    auto started = std::chrono::steady_clock::now();
    auto best    = searcher.bestMove(board, tight);
    auto spent   = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - started).count();

    EXPECT_LT(spent, 2000);
    EXPECT_NE(static_cast<int>(best), 0);
    EXPECT_LT(g_depthSeen, 8);
    EXPECT_EQ(board.fen(), std::string(kiwi));
}

TEST(Search_softLimit, Positive) {

    Board  board;
    Search searcher;

    EXPECT_TRUE(board.setFen(START_POSITION));

    searcher.setWatcher(note);

    g_depthSeen = 0;

    //  a generous ceiling the clock should stop well short of

    searcher.bestMove(board, read("go depth 30 movetime 150", WHITE));

    EXPECT_GT(g_depthSeen, 0);
    EXPECT_LT(g_depthSeen, 30);
    EXPECT_GT(searcher.getNodes(), 0ull);
}

TEST(Search_untimedIsUncut, Positive) {

    Board  board;
    Search searcher;

    EXPECT_TRUE(board.setFen(START_POSITION));

    searcher.setWatcher(note);

    g_depthSeen = 0;

    //  with no clock the ceiling is the only limit, so every iteration must be reached

    searcher.bestMove(board, read("go depth 5", WHITE));

    EXPECT_EQ(g_depthSeen, 5);
}

TEST(Search_depthStillWorks, Positive) {

    Board  board;
    Search searcher;

    EXPECT_TRUE(board.setFen("6k1/5ppp/8/8/8/8/8/R3K2R w KQ - 0 1"));

    EXPECT_EQ(searcher.bestMove(board, 3).toString(), "a1a8");
    EXPECT_EQ(searcher.getScore(), int(MATE_SCORE) - 1);
}

}
pasteque_namespace_end

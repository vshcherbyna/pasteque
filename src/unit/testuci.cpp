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

#include "../uci.h"

pasteque_namespace_begin
namespace unit
{

//
//  Drives the protocol handler directly and captures whatever it writes, so the whole
//  layer is under test without a console on either end
//

static std::string speak(Uci & handler, const std::string & line)
{
    std::ostringstream captured;

    auto previous = std::cout.rdbuf(captured.rdbuf());

    handler.handleCommand(line);

    std::cout.rdbuf(previous);

    return captured.str();
}

static bool mentions(const std::string & text, const std::string & needle)
{
    return text.find(needle) != std::string::npos;
}

//
//  Runs the real startup path with a script standing in for the console, so what the
//  engine says before it is spoken to is under test as well
//

static std::string converse(Uci & handler, const std::string & script)
{
    std::istringstream typed(script);
    std::ostringstream captured;

    auto spoken = std::cout.rdbuf(captured.rdbuf());
    auto heard  = std::cin.rdbuf(typed.rdbuf());

    handler.handleCmdLine(0, nullptr);

    std::cin.rdbuf(heard);
    std::cout.rdbuf(spoken);

    return captured.str();
}

static size_t tally(const std::string & text, const std::string & needle)
{
    size_t total = 0;

    for (auto at = text.find(needle); at != std::string::npos; at = text.find(needle, at + needle.size()))
        ++total;

    return total;
}

TEST(Uci_handshake, Positive)
{
    Uci handler;

    auto greeting = speak(handler, "uci");

    EXPECT_TRUE(mentions(greeting, "id name pasteque"));
    EXPECT_TRUE(mentions(greeting, "id author"));
    EXPECT_TRUE(mentions(greeting, "uciok"));

    //  the name of the other engine used to be here

    EXPECT_FALSE(mentions(greeting, "igel"));

    EXPECT_EQ(speak(handler, "isready"), "readyok\n");
}

TEST(Uci_quit, Positive)
{
    Uci handler;

    EXPECT_FALSE(handler.departing());

    //  stop arrives between searches and there is nothing to stop, it must not end us

    speak(handler, "stop");
    EXPECT_FALSE(handler.departing());

    speak(handler, "quit");
    EXPECT_TRUE(handler.departing());
}

TEST(Uci_position, Positive)
{
    Uci handler;

    speak(handler, "position startpos");
    EXPECT_EQ(handler.getBoard().fen(), START_POSITION);

    speak(handler, "position startpos moves e2e4 e7e5 g1f3");
    EXPECT_EQ(handler.getBoard().fen(),
              "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");

    const char * kiwi = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

    speak(handler, std::string("position fen ") + kiwi);
    EXPECT_EQ(handler.getBoard().fen(), kiwi);

    //  castling through the notation carries the rook, promotion picks the right piece

    speak(handler, std::string("position fen ") + kiwi + " moves e1g1");
    EXPECT_EQ(handler.getBoard().getPiece(G1), WHITE_KING);
    EXPECT_EQ(handler.getBoard().getPiece(F1), WHITE_ROOK);

    speak(handler, "position fen 7k/P7/8/8/8/8/8/4K3 w - - 0 1 moves a7a8n");
    EXPECT_EQ(handler.getBoard().getPiece(A8), WHITE_KNIGHT);

    //  en passant, where the captured pawn is not on the square moved to

    speak(handler, "position fen rnbqkbnr/pp2pppp/8/2ppP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3 moves e5d6");
    EXPECT_EQ(handler.getBoard().getPiece(D6), WHITE_PAWN);
    EXPECT_EQ(handler.getBoard().getPiece(D5), EMPTY);
}

TEST(Uci_position, Negative)
{
    Uci handler;

    //  a move that is not legal is refused and the position is left where it was

    speak(handler, "position startpos moves e2e4 e7e5");
    auto before = handler.getBoard().fen();

    auto complaint = speak(handler, "position startpos moves e2e4 e7e5 e1e3");

    EXPECT_TRUE(mentions(complaint, "info string"));
    EXPECT_EQ(handler.getBoard().fen(), before);

    //  so is a fen that cannot be read

    EXPECT_TRUE(mentions(speak(handler, "position fen not/a/fen w - - 0 1"), "info string"));

    //  and nothing at all is simply ignored

    EXPECT_EQ(speak(handler, ""), "");
    EXPECT_EQ(speak(handler, "   "), "");
    EXPECT_EQ(speak(handler, "nonsense with arguments"), "");
}

TEST(Uci_go, Positive)
{
    Uci handler;

    speak(handler, "position startpos");

    auto thinking = speak(handler, "go depth 3");

    EXPECT_TRUE(mentions(thinking, "info depth 1"));
    EXPECT_TRUE(mentions(thinking, "info depth 3"));
    EXPECT_TRUE(mentions(thinking, "score cp"));
    EXPECT_TRUE(mentions(thinking, "nodes "));
    EXPECT_TRUE(mentions(thinking, "pv "));
    EXPECT_TRUE(mentions(thinking, "bestmove "));

    //  a forced mate is reported in moves, not in centipawns

    speak(handler, "position fen 6k1/5ppp/8/8/8/8/8/R3K2R w KQ - 0 1");

    auto mate = speak(handler, "go depth 3");

    EXPECT_TRUE(mentions(mate, "score mate 1"));
    EXPECT_TRUE(mentions(mate, "bestmove a1a8"));

    //  with no legal move the gui is still owed an answer

    speak(handler, "position fen 7k/8/8/8/8/8/2q5/K7 w - - 0 1");
    EXPECT_TRUE(mentions(speak(handler, "go depth 3"), "bestmove 0000"));

    //  go with no depth still returns, and a silly depth is clamped rather than obeyed

    speak(handler, "position startpos");
    EXPECT_TRUE(mentions(speak(handler, "go"), "bestmove "));
    EXPECT_TRUE(mentions(speak(handler, "go depth 0"), "bestmove "));
    EXPECT_TRUE(mentions(speak(handler, "go depth -5"), "bestmove "));

    //  clock arguments are read and ignored rather than refused

    EXPECT_TRUE(mentions(speak(handler, "go wtime 1000 btime 1000 depth 2"), "bestmove "));
}

TEST(Uci_newGame, Positive)
{
    Uci handler;

    speak(handler, "position startpos moves e2e4");
    speak(handler, "ucinewgame");

    EXPECT_EQ(handler.getBoard().fen(), START_POSITION);
}

//
//  A gui is entitled to hear nothing that looks like a handshake until it asks for one,
//  and to hear it exactly once when it does
//

TEST(Uci_startupIsQuiet, Positive)
{
    Uci handler;

    auto session = converse(handler, "quit\n");

    EXPECT_EQ(tally(session, "uciok"), 0u);
    EXPECT_TRUE(mentions(session, "pasteque"));
}

TEST(Uci_handshakeOnce, Positive)
{
    Uci handler;

    EXPECT_EQ(tally(converse(handler, "uci\nquit\n"), "uciok"), 1u);
}

}
pasteque_namespace_end

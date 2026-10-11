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

#include "../board.h"
#include "../moves.h"

pasteque_namespace_begin
namespace unit
{

TEST(Board, Positive) {
    Board b{};

    EXPECT_EQ(b.fen(), START_POSITION);

    EXPECT_EQ(b.getSide(), WHITE);
    EXPECT_EQ(b.getCastlings(), WHITE_SHORT | WHITE_LONG | BLACK_SHORT | BLACK_LONG);
    EXPECT_EQ(b.getEp(), NO_SQUARE);
    EXPECT_EQ(b.getFifty(), 0u);
    EXPECT_EQ(b.getMoveNumber(), 1u);

    EXPECT_EQ(b.getPiece(E1), WHITE_KING);
    EXPECT_EQ(b.getPiece(E8), BLACK_KING);
    EXPECT_EQ(b.getPiece(C8), BLACK_BISHOP);
    EXPECT_EQ(b.getPiece(E4), EMPTY);
}

TEST(Board_clear, Positive) {
    Board b{};

    b.clear();

    EXPECT_EQ(b.getAllPieces(), 0ULL);
    EXPECT_EQ(b.fen(), "8/8/8/8/8/8/8/8 w - - 0 1");
}

TEST(Board_setFen, Positive) {
    static const char * FENS[] = {
        START_POSITION,
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
        "4k3/8/8/8/8/8/8/4K3 b - - 99 175"
    };

    for (auto fen : FENS) {
        Board b{};

        EXPECT_TRUE(b.setFen(fen));
        EXPECT_EQ(b.fen(), fen);
    }
}

TEST(Board_setFen, Negative) {
    static const char * FENS[] = {
        "",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN w KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNRR w KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBXR w KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQxq - 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq j9 0 1"
    };

    for (auto fen : FENS) {
        Board b{};

        EXPECT_FALSE(b.setFen(fen));
    }
}

TEST(Board_setFen, Occupancy) {
    Board b{};

    EXPECT_TRUE(b.setFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"));

    bitboard white = 0,
             black = 0;

    for (auto square = 0; square < 64; ++square) {
        auto piece = b.getPiece(square);

        if (piece == EMPTY)
            continue;

        EXPECT_NE(b.getPieces(piece) & (1ULL << square), 0ULL);

        if (piece_color(piece) == WHITE)
            bit_set(white, square);
        else
            bit_set(black, square);
    }

    EXPECT_EQ(b.getAllPieces(WHITE), white);
    EXPECT_EQ(b.getAllPieces(BLACK), black);
    EXPECT_EQ(b.getAllPieces(), white | black);
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

static Move find(Board & board, const char * notation) {
    Moves moves;

    moves.generateLegal(board);

    for (auto i = 0; i < moves.size(); ++i)
        if (moves[i].toString() == notation)
            return moves[i];

    return Move();
}

static const char * MOVE_FENS[] = {
    START_POSITION,
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
    "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
    "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1",
    "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1",
    "r3k2r/6B1/8/8/8/8/8/R3K2R w KQkq - 0 1",
    "r3k2r/8/8/8/8/8/6b1/R3K2R b KQkq - 0 1"
};

TEST(Board_stamp, MatchesFreshParseAfterEveryMove) {
    for (auto fen : MOVE_FENS) {
        Board b{};

        ASSERT_TRUE(b.setFen(fen)) << fen;

        Moves moves;
        moves.generateLegal(b);

        ASSERT_NE(moves.size(), 0) << fen;

        for (auto i = 0; i < moves.size(); ++i) {
            Rewind undo;

            b.doMove(moves[i], undo);

            Board parsed{};

            ASSERT_TRUE(parsed.setFen(b.fen())) << fen << " " << moves[i].toString();
            EXPECT_EQ(b.getStamp(), parsed.getStamp()) << fen << " " << moves[i].toString();

            b.unmakeMove(moves[i], undo);
        }
    }
}

TEST(Board_null, MatchesAFreshParse) {
    for (auto fen : MOVE_FENS) {
        Board b{};

        ASSERT_TRUE(b.setFen(fen)) << fen;

        Rewind undo;

        b.doNull(undo);

        Board parsed{};

        ASSERT_TRUE(parsed.setFen(b.fen())) << fen;
        EXPECT_EQ(b.getStamp(), parsed.getStamp()) << fen;

        b.unmakeNull(undo);
    }
}

TEST(Board_null, RecordsThePositionItPassedFrom) {
    for (auto fen : MOVE_FENS) {
        Board b{};

        ASSERT_TRUE(b.setFen(fen)) << fen;

        Rewind first,
               second;

        b.doNull(first);
        b.doNull(second);

        EXPECT_EQ(first.m_trailPly + 1, second.m_trailPly) << fen;

        b.unmakeNull(second);
        b.unmakeNull(first);

        EXPECT_EQ(first.m_trailPly, second.m_trailPly - 1) << fen;
    }
}

TEST(Board_null, RestoresEverything) {
    for (auto fen : MOVE_FENS) {
        Board b{};

        ASSERT_TRUE(b.setFen(fen)) << fen;

        auto before = b.fen();
        auto marked = b.getStamp();

        Rewind undo;

        b.doNull(undo);
        b.unmakeNull(undo);

        EXPECT_EQ(before, b.fen()) << fen;
        EXPECT_EQ(marked, b.getStamp()) << fen;
    }
}

TEST(Board_null, PassesTheTurnAndGivesUpEnPassant) {
    Board b{};

    ASSERT_TRUE(b.setFen("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2"));

    auto side  = b.getSide();
    auto fifty = b.getFifty();

    Rewind undo;

    b.doNull(undo);

    EXPECT_NE(side, b.getSide());
    EXPECT_EQ(NO_SQUARE, b.getEp());
    EXPECT_EQ(fifty, b.getFifty());
    EXPECT_NE(undo.m_stamp, b.getStamp());

    b.unmakeNull(undo);

    EXPECT_EQ(side, b.getSide());
    EXPECT_NE(NO_SQUARE, b.getEp());
}

TEST(Board_null, CountsTheMoveNumberLikeARealMove) {
    Board b{};

    ASSERT_TRUE(b.setFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1"));

    auto number = b.getMoveNumber();

    Rewind undo;

    b.doNull(undo);

    EXPECT_EQ(number + 1, b.getMoveNumber());

    b.unmakeNull(undo);

    EXPECT_EQ(number, b.getMoveNumber());
}

TEST(Board_null, RepetitionDoesNotReachAcrossThePass) {
    Board b{};

    ASSERT_TRUE(b.setFen("r3k3/8/8/8/8/8/8/3K2N1 w - - 0 1"));

    static const char * LINE[] = { "d1e1", "e8f8", "g1f3", "f8f7", "f3g1", "f7e8" };

    for (auto notation : LINE)
        ASSERT_TRUE(play(b, notation)) << notation;

    ASSERT_FALSE(b.recurred(6));

    Rewind undo;

    b.doNull(undo);

    EXPECT_FALSE(b.recurred(7));

    b.unmakeNull(undo);

    EXPECT_FALSE(b.recurred(6));
}

TEST(Board_null, TheClockDoesNotTickOnAPass) {
    Board b{};

    ASSERT_TRUE(b.setFen("7k/6pr/5N1p/5N2/8/8/8/R3K3 b - - 99 1"));

    Rewind undo;

    b.doNull(undo);

    EXPECT_EQ(99u, b.getFifty());

    b.unmakeNull(undo);

    EXPECT_EQ(99u, b.getFifty());
}

TEST(Board_onlyPawns, Positive) {
    Board b{};

    ASSERT_TRUE(b.setFen("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1"));

    EXPECT_TRUE(b.onlyPawns(WHITE));
    EXPECT_TRUE(b.onlyPawns(BLACK));

    ASSERT_TRUE(b.setFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1"));

    EXPECT_TRUE(b.onlyPawns(WHITE));
    EXPECT_TRUE(b.onlyPawns(BLACK));
}

TEST(Board_onlyPawns, Negative) {
    Board b{};

    EXPECT_FALSE(b.onlyPawns(WHITE));
    EXPECT_FALSE(b.onlyPawns(BLACK));

    ASSERT_TRUE(b.setFen("4k3/pppppppp/8/8/8/8/PPPPPPPP/4KN2 w - - 0 1"));

    EXPECT_FALSE(b.onlyPawns(WHITE));
    EXPECT_TRUE(b.onlyPawns(BLACK));
}

TEST(Board_stamp, SurvivesUnmake) {
    for (auto fen : MOVE_FENS) {
        Board b{};

        EXPECT_TRUE(b.setFen(fen)) << fen;

        auto before = b.getStamp();

        EXPECT_NE(before, 0ULL) << fen;

        Moves moves;
        moves.generateLegal(b);

        for (auto i = 0; i < moves.size(); ++i) {
            Rewind undo;

            b.doMove(moves[i], undo);

            EXPECT_NE(b.getStamp(), before) << fen << " " << moves[i].toString();

            b.unmakeMove(moves[i], undo);

            EXPECT_EQ(b.getStamp(), before) << fen << " " << moves[i].toString();
            EXPECT_EQ(b.fen(), std::string(fen)) << fen;
        }
    }
}

TEST(Board_stamp, IgnoresMoveOrder) {
    Board first{},
          second{};

    EXPECT_TRUE(play(first, "g1f3"));
    EXPECT_TRUE(play(first, "g8f6"));
    EXPECT_TRUE(play(first, "b1c3"));
    EXPECT_TRUE(play(first, "b8c6"));

    EXPECT_TRUE(play(second, "b1c3"));
    EXPECT_TRUE(play(second, "b8c6"));
    EXPECT_TRUE(play(second, "g1f3"));
    EXPECT_TRUE(play(second, "g8f6"));

    EXPECT_EQ(first.fen(), second.fen());
    EXPECT_EQ(first.getStamp(), second.getStamp());
}

TEST(Board_stamp, MatchesSetFen) {
    static const char * MOVES[] = { "e2e4", "c7c5", "g1f3", "d7d6", "f1b5", "c8d7", "e1g1" };

    Board played{};

    for (auto notation : MOVES) {
        EXPECT_TRUE(play(played, notation)) << notation;

        Board parsed{};

        EXPECT_TRUE(parsed.setFen(played.fen())) << notation;
        EXPECT_EQ(parsed.getStamp(), played.getStamp()) << notation;
    }
}

TEST(Board_stamp, IgnoresUncapturableEnPassant) {
    Board b{};

    EXPECT_TRUE(b.setFen("4k1n1/8/8/8/8/8/4P3/4K1N1 w - - 0 1"));
    EXPECT_TRUE(play(b, "e2e4"));

    auto pushed = b.getStamp();

    EXPECT_TRUE(play(b, "g8f6"));
    EXPECT_TRUE(play(b, "g1f3"));
    EXPECT_TRUE(play(b, "f6g8"));
    EXPECT_TRUE(play(b, "f3g1"));

    EXPECT_EQ(b.getStamp(), pushed);
    EXPECT_TRUE(b.recurred(5));
}

TEST(Board_stamp, IgnoresPinnedEnPassant) {
    Board offered{},
          spent{};

    EXPECT_TRUE(offered.setFen("k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 2"));
    EXPECT_TRUE(spent.setFen("k3r3/8/8/3pP3/8/8/8/4K3 w - - 0 2"));

    Moves moves;
    moves.generateLegal(offered);

    for (auto i = 0; i < moves.size(); ++i)
        EXPECT_NE(moves[i].toString(), "e5d6");

    EXPECT_EQ(offered.getStamp(), spent.getStamp());
}

TEST(Board_stamp, IgnoresCheckedEnPassant) {
    Board offered{},
          spent{};

    EXPECT_TRUE(offered.setFen("k7/8/8/3pP3/8/6n1/8/7K w - d6 0 2"));
    EXPECT_TRUE(spent.setFen("k7/8/8/3pP3/8/6n1/8/7K w - - 0 2"));

    Moves moves;
    moves.generateLegal(offered);

    for (auto i = 0; i < moves.size(); ++i)
        EXPECT_NE(moves[i].toString(), "e5d6");

    EXPECT_EQ(offered.getStamp(), spent.getStamp());
}

TEST(Board_stamp, KeepsCheckBreakingEnPassant) {
    Board offered{},
          spent{};

    EXPECT_TRUE(offered.setFen("k7/8/8/3pP3/4K3/8/8/8 w - d6 0 2"));
    EXPECT_TRUE(spent.setFen("k7/8/8/3pP3/4K3/8/8/8 w - - 0 2"));

    Moves moves;
    moves.generateLegal(offered);

    auto found = false;

    for (auto i = 0; i < moves.size(); ++i)
        if (moves[i].toString() == "e5d6")
            found = true;

    EXPECT_TRUE(found);
    EXPECT_NE(offered.getStamp(), spent.getStamp());
}

TEST(Board_stamp, KeepsCapturableEnPassant) {
    Board offered{},
          spent{};

    EXPECT_TRUE(offered.setFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2"));
    EXPECT_TRUE(spent.setFen("4k3/8/8/3pP3/8/8/8/4K3 w - - 0 2"));

    EXPECT_NE(offered.getStamp(), spent.getStamp());
}

static void shuffle(Board & board) {
    EXPECT_TRUE(play(board, "g1f3"));
    EXPECT_TRUE(play(board, "g8f6"));
    EXPECT_TRUE(play(board, "f3g1"));
    EXPECT_TRUE(play(board, "f6g8"));
}

TEST(Board_recurred, ThreefoldAtRoot) {
    Board b{};

    shuffle(b);

    EXPECT_EQ(b.fen(), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 4 3");
    EXPECT_FALSE(b.recurred(0));

    shuffle(b);

    EXPECT_TRUE(b.recurred(0));
}

TEST(Board_recurred, TwofoldAfterRoot) {
    Board b{};

    shuffle(b);

    EXPECT_FALSE(b.recurred(4));
    EXPECT_TRUE(b.recurred(5));
}

TEST(Board_recurred, Negative) {
    Board b{};

    EXPECT_FALSE(b.recurred(0));

    EXPECT_TRUE(play(b, "g1f3"));
    EXPECT_FALSE(b.recurred(0));

    EXPECT_TRUE(play(b, "g8f6"));
    EXPECT_FALSE(b.recurred(0));

    EXPECT_TRUE(play(b, "f3g1"));
    EXPECT_FALSE(b.recurred(0));
}

TEST(Board_recurred, ForgetsBeforePawnMove) {
    Board b{};

    shuffle(b);
    shuffle(b);

    EXPECT_TRUE(b.recurred(0));

    EXPECT_TRUE(play(b, "e2e4"));

    EXPECT_FALSE(b.recurred(0));
}

TEST(Board_recurred, SurvivesExploredIrreversibleMove) {
    Board b{};

    shuffle(b);
    shuffle(b);

    EXPECT_TRUE(b.recurred(0));

    auto fen   = b.fen();
    auto stamp = b.getStamp();

    auto push = find(b, "e2e4");
    ASSERT_NE(static_cast<int>(push), 0);

    Rewind onPush;
    b.doMove(push, onPush);

    auto reply = find(b, "g8f6");
    ASSERT_NE(static_cast<int>(reply), 0);

    Rewind onReply;
    b.doMove(reply, onReply);

    b.unmakeMove(reply, onReply);
    b.unmakeMove(push, onPush);

    EXPECT_EQ(b.fen(), fen);
    EXPECT_EQ(b.getStamp(), stamp);
    EXPECT_TRUE(b.recurred(0));
}

TEST(Board_setFen, RejectsMissingKings) {
    Board b{};

    EXPECT_FALSE(b.setFen("8/8/8/8/8/8/8/8 w - a3 0 1"));
    EXPECT_FALSE(b.setFen("8/8/8/3pP3/8/8/8/8 w - d6 0 1"));
    EXPECT_FALSE(b.setFen("4k3/8/8/8/8/8/8/8 w - - 0 1"));
    EXPECT_FALSE(b.setFen("4k3/8/8/8/8/8/8/3KK3 w - - 0 1"));
}

}
pasteque_namespace_end

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

#include "../perft.h"
#include "../moves.h"

pasteque_namespace_begin
namespace unit
{

static unsigned long long nodes(const char * fen, int depth)
{
    Board board;

    EXPECT_TRUE(board.setFen(fen));

    auto counted = perft(board, depth);

    //  make and unmake must leave the position exactly as it was found

    EXPECT_EQ(board.fen(), std::string(fen));

    return counted;
}

//
//  The six standard perft positions. Depths are kept shallow so the suite stays quick,
//  the deep runs live in the benchmark rather than here
//

TEST(Perft, Initial)
{
    EXPECT_EQ(nodes(START_POSITION, 1), 20ULL);
    EXPECT_EQ(nodes(START_POSITION, 2), 400ULL);
    EXPECT_EQ(nodes(START_POSITION, 3), 8902ULL);
    EXPECT_EQ(nodes(START_POSITION, 4), 197281ULL);
    EXPECT_EQ(nodes(START_POSITION, 5), 4865609ULL);
}

TEST(Perft, Kiwipete)
{
    const char * fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

    EXPECT_EQ(nodes(fen, 1), 48ULL);
    EXPECT_EQ(nodes(fen, 2), 2039ULL);
    EXPECT_EQ(nodes(fen, 3), 97862ULL);
    EXPECT_EQ(nodes(fen, 4), 4085603ULL);
}

TEST(Perft, Endgame)
{
    const char * fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";

    EXPECT_EQ(nodes(fen, 1), 14ULL);
    EXPECT_EQ(nodes(fen, 4), 43238ULL);
    EXPECT_EQ(nodes(fen, 5), 674624ULL);
}

TEST(Perft, Promotions)
{
    const char * fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";

    EXPECT_EQ(nodes(fen, 1), 6ULL);
    EXPECT_EQ(nodes(fen, 4), 422333ULL);
}

TEST(Perft, Mirrored)
{
    EXPECT_EQ(nodes("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4), 2103487ULL);
    EXPECT_EQ(nodes("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4),
              3894594ULL);
}

TEST(Board_isAttacked, Positive)
{
    Board board;

    //  the opening position, every square of the third rank is covered by white

    EXPECT_TRUE(board.isAttacked(A3, WHITE));
    EXPECT_TRUE(board.isAttacked(H3, WHITE));
    EXPECT_TRUE(board.isAttacked(E2, WHITE));
    EXPECT_FALSE(board.isAttacked(E4, WHITE));
    EXPECT_FALSE(board.isAttacked(E5, WHITE));

    EXPECT_FALSE(board.inCheck());

    EXPECT_TRUE(board.setFen("4k3/8/8/8/8/8/8/4K2R b K - 0 1"));
    EXPECT_FALSE(board.inCheck());

    EXPECT_TRUE(board.setFen("4k3/8/8/8/8/8/8/4R1K1 b - - 0 1"));
    EXPECT_TRUE(board.inCheck());
}

TEST(Board_makeMove, Positive)
{
    Board board;
    Rewind  undo;

    //  a castling move carries the rook with it, and taking it back restores both

    EXPECT_TRUE(board.setFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"));
    EXPECT_TRUE(board.makeMove(Move(E1, G1, WHITE_KING, EMPTY, EMPTY, MOVE_CASTLING), undo));
    EXPECT_EQ(board.fen(), "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1");

    board.unmakeMove(Move(E1, G1, WHITE_KING, EMPTY, EMPTY, MOVE_CASTLING), undo);
    EXPECT_EQ(board.fen(), "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

    //  a king may not castle out of check

    EXPECT_TRUE(board.setFen("r3k2r/8/8/8/8/8/4r3/R3K2R w KQkq - 0 1"));
    EXPECT_FALSE(board.makeMove(Move(E1, G1, WHITE_KING, EMPTY, EMPTY, MOVE_CASTLING), undo));
    EXPECT_EQ(board.fen(), "r3k2r/8/8/8/8/8/4r3/R3K2R w KQkq - 0 1");

    //  nor through an attacked square

    EXPECT_TRUE(board.setFen("r3k2r/8/8/8/8/8/5r2/R3K2R w KQkq - 0 1"));
    EXPECT_FALSE(board.makeMove(Move(E1, G1, WHITE_KING, EMPTY, EMPTY, MOVE_CASTLING), undo));

    //  a double push sets the ep square, a quiet move clears it

    EXPECT_TRUE(board.setFen(START_POSITION));
    EXPECT_TRUE(board.makeMove(Move(E2, E4, WHITE_PAWN), undo));
    EXPECT_EQ(board.getEp(), E3);

    EXPECT_TRUE(board.makeMove(Move(B8, C6, BLACK_KNIGHT), undo));
    EXPECT_EQ(board.getEp(), NO_SQUARE);
}

}
pasteque_namespace_end

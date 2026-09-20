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

#include "../board.h"

pasteque_namespace_begin
namespace unit
{

TEST(Board, Positive)
{
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

TEST(Board_clear, Positive)
{
    Board b{};

    b.clear();

    EXPECT_EQ(b.getAllPieces(), 0ULL);
    EXPECT_EQ(b.fen(), "8/8/8/8/8/8/8/8 w - - 0 1");
}

TEST(Board_setFen, Positive)
{
    static const char * FENS[] = {
        START_POSITION,
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
        "4k3/8/8/8/8/8/8/4K3 b - - 99 175"
    };

    for (auto fen : FENS)
    {
        Board b{};

        EXPECT_TRUE(b.setFen(fen));
        EXPECT_EQ(b.fen(), fen);
    }
}

TEST(Board_setFen, Negative)
{
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

    for (auto fen : FENS)
    {
        Board b{};

        EXPECT_FALSE(b.setFen(fen));
    }
}

TEST(Board_setFen, Occupancy)
{
    Board b{};

    EXPECT_TRUE(b.setFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"));

    bitboard white = 0,
             black = 0;

    for (auto square = 0; square < 64; ++square)
    {
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

}
pasteque_namespace_end

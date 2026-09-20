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

#include <set>
#include <string>

#include "../moves.h"
#include "../attacks.h"

pasteque_namespace_begin
namespace unit
{

static int countGenerated(const char * fen)
{
    Board board;
    Moves moves;

    EXPECT_TRUE(board.setFen(fen));
    moves.generate(board);

    return moves.size();
}

static int countLegal(const char * fen)
{
    Board board;
    Moves moves;

    EXPECT_TRUE(board.setFen(fen));
    moves.generateLegal(board);

    return moves.size();
}

//
//  The fast path has to agree with the slow one everywhere. generateLegal decides
//  legality from pins and check masks without touching the board, makeMove decides it by
//  playing the move and looking - and that is the path perft already vouches for
//

static bool agreesWithMakeAndTest(Board & board, int depth)
{
    Moves legal, pseudo;

    legal.generateLegal(board);
    pseudo.generate(board);

    std::set<int> fast, slow;

    for (auto i = 0; i < legal.size(); ++i)
        fast.insert(legal[i]);

    for (auto i = 0; i < pseudo.size(); ++i)
    {
        Rewind undo;

        if (board.makeMove(pseudo[i], undo))
        {
            slow.insert(pseudo[i]);
            board.unmakeMove(pseudo[i], undo);
        }
    }

    if (fast != slow)
        return false;

    if (depth <= 1)
        return true;

    for (auto i = 0; i < legal.size(); ++i)
    {
        Rewind undo;

        board.doMove(legal[i], undo);

        auto ok = agreesWithMakeAndTest(board, depth - 1);

        board.unmakeMove(legal[i], undo);

        if (!ok)
            return false;
    }

    return true;
}

static bool generatedLegal(const char * fen, const std::string & lan)
{
    Board board;
    Moves moves;

    EXPECT_TRUE(board.setFen(fen));
    moves.generateLegal(board);

    for (auto i = 0; i < moves.size(); ++i)
        if (moves[i].toString() == lan)
            return true;

    return false;
}

static bool generated(const char * fen, const std::string & lan)
{
    Board board;
    Moves moves;

    EXPECT_TRUE(board.setFen(fen));
    moves.generate(board);

    for (auto i = 0; i < moves.size(); ++i)
        if (moves[i].toString() == lan)
            return true;

    return false;
}

TEST(Attacks, Positive)
{
    initAttacks();

    EXPECT_EQ(popCount(KNIGHT_ATTACKS[E4]), 8);
    EXPECT_EQ(popCount(KNIGHT_ATTACKS[A1]), 2);
    EXPECT_EQ(popCount(KING_ATTACKS[E4]), 8);
    EXPECT_EQ(popCount(KING_ATTACKS[A1]), 3);
    EXPECT_EQ(popCount(PAWN_ATTACKS[WHITE][E2]), 2);
    EXPECT_EQ(popCount(PAWN_ATTACKS[WHITE][A2]), 1);
    EXPECT_EQ(popCount(PAWN_ATTACKS[BLACK][E7]), 2);

    //  a rook alone on an empty board reaches fourteen squares, a bishop on e4 thirteen

    EXPECT_EQ(popCount(rookAttacks(A1, 0)), 14);
    EXPECT_EQ(popCount(bishopAttacks(E4, 0)), 13);
    EXPECT_EQ(popCount(queenAttacks(E4, 0)), 27);

    //  a blocker stops the ray on the square it stands on

    EXPECT_EQ(popCount(rookAttacks(A1, bit_of(A4))), 10);
}

TEST(MoveGen, Positive)
{
    //  the six standard perft positions, pseudo legal counts

    EXPECT_EQ(countGenerated(START_POSITION), 20);
    EXPECT_EQ(countGenerated("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"), 16);
    EXPECT_EQ(countGenerated("4k3/8/8/8/8/8/8/4K3 w - - 0 1"), 5);

    //  a lone knight in the corner, and a king that may not move into its own pieces

    EXPECT_EQ(countGenerated("4k3/8/8/8/8/8/8/N3K3 w - - 0 1"), 7);
}

TEST(MoveGen_promotions, Positive)
{
    //  one pawn on the seventh, four promotions pushing plus four capturing

    EXPECT_EQ(countGenerated("7k/4P3/8/8/8/8/8/4K3 w - - 0 1"), 4 + 5);

    EXPECT_TRUE(generated("7k/4P3/8/8/8/8/8/4K3 w - - 0 1", "e7e8q"));
    EXPECT_TRUE(generated("7k/4P3/8/8/8/8/8/4K3 w - - 0 1", "e7e8n"));
    EXPECT_TRUE(generated("7k/4P3/8/8/8/8/8/4K3 w - - 0 1", "e7e8r"));
    EXPECT_TRUE(generated("7k/4P3/8/8/8/8/8/4K3 w - - 0 1", "e7e8b"));
}

TEST(MoveGen_castling, Positive)
{
    const char * fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";

    EXPECT_TRUE(generated(fen, "e1g1"));
    EXPECT_TRUE(generated(fen, "e1c1"));

    //  without the rights they are gone

    EXPECT_FALSE(generated("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1", "e1g1"));
    EXPECT_FALSE(generated("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1", "e1c1"));

    //  and a piece in the way blocks them

    EXPECT_FALSE(generated("r3k2r/8/8/8/8/8/8/R3K1NR w KQkq - 0 1", "e1g1"));
    EXPECT_FALSE(generated("r3k2r/8/8/8/8/8/8/RN2K2R w KQkq - 0 1", "e1c1"));
}

TEST(MoveGen_enPassant, Positive)
{
    EXPECT_TRUE(generated("rnbqkbnr/pp2pppp/8/2ppP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3", "e5d6"));
    EXPECT_FALSE(generated("rnbqkbnr/pp2pppp/8/2ppP3/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3", "e5d6"));
}

TEST(Move_toString, Positive)
{
    EXPECT_EQ(Move(E2, E4, WHITE_PAWN).toString(), "e2e4");
    EXPECT_EQ(Move(A1, H8, WHITE_QUEEN).toString(), "a1h8");
    EXPECT_EQ(Move(E7, E8, WHITE_PAWN, EMPTY, WHITE_QUEEN).toString(), "e7e8q");
    EXPECT_EQ(Move(B7, A8, WHITE_PAWN, BLACK_ROOK, WHITE_KNIGHT).toString(), "b7a8n");
}

TEST(Move_fields, Positive)
{
    Move m(E2, E4, WHITE_PAWN, BLACK_KNIGHT, WHITE_QUEEN, MOVE_ENPASSANT);

    EXPECT_EQ(m.getFrom(), E2);
    EXPECT_EQ(m.getTo(), E4);
    EXPECT_EQ(m.getPiece(), WHITE_PAWN);
    EXPECT_EQ(m.getCapture(), BLACK_KNIGHT);
    EXPECT_EQ(m.getPromotion(), WHITE_QUEEN);
    EXPECT_TRUE(m.isEnPassant());
    EXPECT_FALSE(m.isCastling());
}


TEST(MoveGen_legal, Positive)
{
    //  the six standard perft positions at depth one

    EXPECT_EQ(countLegal(START_POSITION), 20);
    EXPECT_EQ(countLegal("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"), 48);
    EXPECT_EQ(countLegal("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"), 14);
    EXPECT_EQ(countLegal("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"), 6);
    EXPECT_EQ(countLegal("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"), 44);
    EXPECT_EQ(countLegal("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"), 46);

    //  a knight pinned against its king on the e file cannot move at all, so only the
    //  four king moves remain - and the generator does still offer the knight pseudo
    //  legally, which is the whole difference between the two entry points

    EXPECT_EQ(countLegal("4r2k/8/8/8/8/8/4N3/4K3 w - - 0 1"), 4);
    EXPECT_FALSE(generatedLegal("4r2k/8/8/8/8/8/4N3/4K3 w - - 0 1", "e2c3"));
    EXPECT_TRUE(generated("4r2k/8/8/8/8/8/4N3/4K3 w - - 0 1", "e2c3"));

    //  under a double check nothing but the king may move, here including a capture of
    //  one of the two checkers

    EXPECT_EQ(countLegal("4k3/8/8/8/8/3n4/4r3/4K3 w - - 0 1"), 3);
    EXPECT_TRUE(generatedLegal("4k3/8/8/8/8/3n4/4r3/4K3 w - - 0 1", "e1e2"));

    //  mate and stalemate both come out as no legal moves, told apart by the checkers

    EXPECT_EQ(countLegal("6k1/8/8/8/8/8/5PPP/4r1K1 w - - 0 1"), 0);
    EXPECT_EQ(countLegal("7k/8/8/8/8/8/5q2/7K w - - 0 1"), 0);
    EXPECT_EQ(countLegal("7k/8/8/8/8/8/2q5/K7 w - - 0 1"), 0);

    Board board;

    EXPECT_TRUE(board.setFen("6k1/8/8/8/8/8/5PPP/4r1K1 w - - 0 1"));
    EXPECT_TRUE(board.getCheckers() != 0);

    EXPECT_TRUE(board.setFen("7k/8/8/8/8/8/2q5/K7 w - - 0 1"));
    EXPECT_TRUE(board.getCheckers() == 0);
}

TEST(MoveGen_legal, AgreesWithMakeAndTest)
{
    static const char * FENS[] = {
        START_POSITION,
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "rnbqkbnr/pp2pppp/8/2ppP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
        "8/8/3p4/1Pp4r/1K3p1k/8/4P1P1/1R6 w - c6 0 3"
    };

    for (auto fen : FENS)
    {
        Board board;

        EXPECT_TRUE(board.setFen(fen));
        EXPECT_TRUE(agreesWithMakeAndTest(board, 3)) << fen;
        EXPECT_EQ(board.fen(), std::string(fen));
    }
}
}
pasteque_namespace_end

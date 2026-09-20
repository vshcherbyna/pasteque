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
#include <vector>

#include "../judge.h"
#include "../search.h"

pasteque_namespace_begin
namespace unit
{

static const char * POSITIONS[] = {
    START_POSITION,
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "rnbqkbnr/pp2pppp/8/2ppP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
    "8/8/8/4k3/8/2K5/8/8 w - - 10 40",
    "4k3/pp6/8/8/8/8/6PP/4K3 b - - 0 30"
};

//
//  The same position with the two sides exchanged: ranks reversed, piece letters swapped
//  in case, side to move and castling rights swapped, ep square reflected
//

static std::string mirror(const std::string & fen)
{
    std::vector<std::string> fields;
    std::string field;

    for (auto ch : fen + " ")
    {
        if (ch == ' ')
        {
            if (!field.empty())
                fields.push_back(field);
            field.clear();
        }
        else
            field += ch;
    }

    std::vector<std::string> ranks;
    std::string rank;

    for (auto ch : fields[0] + "/")
    {
        if (ch == '/')
        {
            ranks.push_back(rank);
            rank.clear();
        }
        else
            rank += ch;
    }

    std::string board;

    for (auto i = ranks.size(); i > 0; --i)
    {
        if (!board.empty())
            board += '/';

        for (auto ch : ranks[i - 1])
        {
            if (ch >= 'a' && ch <= 'z')
                board += static_cast<char>(ch - 'a' + 'A');
            else if (ch >= 'A' && ch <= 'Z')
                board += static_cast<char>(ch - 'A' + 'a');
            else
                board += ch;
        }
    }

    //  swapped in case and put back into the order fen() writes them

    std::string castlings;

    if (fields[2].find('k') != std::string::npos) castlings += 'K';
    if (fields[2].find('q') != std::string::npos) castlings += 'Q';
    if (fields[2].find('K') != std::string::npos) castlings += 'k';
    if (fields[2].find('Q') != std::string::npos) castlings += 'q';

    if (castlings.empty())
        castlings = "-";

    auto ep = fields[3];

    if (ep != "-")
        ep[1] = static_cast<char>('1' + '8' - ep[1]);

    auto side = (fields[1] == "w") ? std::string("b") : std::string("w");

    auto result = board + " " + side + " " + castlings + " " + ep;

    for (size_t i = 4; i < fields.size(); ++i)
        result += " " + fields[i];

    return result;
}

static int evaluate(const std::string & fen)
{
    Board board;

    EXPECT_TRUE(board.setFen(fen)) << fen;

    return Judge::evaluate(board);
}

//
//  The single most useful test an evaluation has. Any term that treats one colour
//  differently from the other shows up here and nowhere else
//

TEST(Judge, Symmetry)
{
    Judge::init();

    for (auto fen : POSITIONS)
        EXPECT_EQ(evaluate(fen), evaluate(mirror(fen))) << fen << "  ->  " << mirror(fen);
}

TEST(Judge_mirror, Positive)
{
    //  the helper the symmetry test leans on has to be right itself

    EXPECT_EQ(mirror(START_POSITION), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    EXPECT_EQ(mirror("rnbqkbnr/pp2pppp/8/2ppP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3"),
              "rnbqkbnr/pppp1ppp/8/8/2PPp3/8/PP2PPPP/RNBQKBNR b KQkq d3 0 3");
}

TEST(Judge, Positive)
{
    Judge::init();

    //  an untouched opening position is level, the side to move is up a tempo

    EXPECT_EQ(evaluate(START_POSITION), int(TEMPO));

    //  the terms are drawn at random, so the evaluation owes us nothing about who is
    //  winning. What it does owe us is distance from the window the search reads as a mate

    for (auto fen : POSITIONS)
    {
        auto eval = evaluate(fen);

        EXPECT_LT((eval < 0) ? -eval : eval, int(MATE_SCORE) - int(PLY_LIMIT)) << fen;
    }

    //  material that cannot mate is drawn whatever the placement says

    EXPECT_EQ(evaluate("4k3/8/8/8/8/8/8/4K3 w - - 0 1"), int(EVEN_SCORE));
    EXPECT_EQ(evaluate("4k3/8/8/8/8/8/8/3NK3 w - - 0 1"), int(EVEN_SCORE));
    EXPECT_EQ(evaluate("4k3/8/8/8/8/8/8/3BK3 b - - 0 1"), int(EVEN_SCORE));

}

//
//  With the tables random there is no chess left to assert, so what is tested instead is
//  that evaluate() really is the sum of the tables it claims to be
//

TEST(Judge_evaluate, MatchesTheTables)
{
    Judge::init();

    for (auto fen : POSITIONS)
    {
        Board board;

        EXPECT_TRUE(board.setFen(fen)) << fen;

        auto tabled  = Judge::pieceSquare(board);
        auto left    = Judge::phase(board);
        auto tapered = (tabled.opening * left + tabled.closing * (int(PHASE_MAX) - left)) / int(PHASE_MAX);

        if (board.getSide() == BLACK)
            tapered = -tapered;

        tapered += int(TEMPO);

        //  material that cannot mate is folded back to level whatever the tables said

        auto actual = Judge::evaluate(board);

        EXPECT_TRUE(actual == tapered || actual == int(EVEN_SCORE))
            << fen << "  " << actual << " vs " << tapered;
    }
}

TEST(Judge_phase, Positive)
{
    Board board;

    EXPECT_TRUE(board.setFen(START_POSITION));
    EXPECT_EQ(Judge::phase(board), int(PHASE_MAX));

    //  both kings are always on the board, so their weight is held at zero and a bare
    //  king ending is the one position that reaches the closing end of the taper

    EXPECT_TRUE(board.setFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1"));
    EXPECT_EQ(Judge::phase(board), 0);

    //  every other position sits somewhere between the two ends

    EXPECT_TRUE(board.setFen("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1"));
    EXPECT_GE(Judge::phase(board), 0);
    EXPECT_LE(Judge::phase(board), int(PHASE_MAX));

    EXPECT_TRUE(board.setFen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1"));
    EXPECT_GE(Judge::phase(board), 0);
    EXPECT_LE(Judge::phase(board), int(PHASE_MAX));
}

TEST(Judge_pieceSquare, Positive)
{
    Judge::init();

    Board board;

    //  the opening position cancels out exactly

    EXPECT_TRUE(board.setFen(START_POSITION));
    EXPECT_EQ(Judge::pieceSquare(board).opening, 0);
    EXPECT_EQ(Judge::pieceSquare(board).closing, 0);

    //  the tables carry no opinion any more, but they still have to be filled in rather
    //  than left flat, and every entry has to sit inside the range the draw promised

    auto varies = false;

    for (auto square = 1; square < 64; ++square)
        if (PIECE_SQUARE[WHITE_KNIGHT][square].opening != PIECE_SQUARE[WHITE_KNIGHT][0].opening)
            varies = true;

    EXPECT_TRUE(varies);

    for (auto square = 0; square < 64; ++square)
    {
        EXPECT_LE(PIECE_SQUARE[WHITE_QUEEN][square].opening,  512);
        EXPECT_GE(PIECE_SQUARE[WHITE_QUEEN][square].opening, -512);
        EXPECT_LE(PIECE_SQUARE[WHITE_QUEEN][square].closing,  512);
        EXPECT_GE(PIECE_SQUARE[WHITE_QUEEN][square].closing, -512);
    }

    //  black's tables are white's, mirrored

    for (auto square = 0; square < 64; ++square)
    {
        EXPECT_EQ(PIECE_SQUARE[WHITE_KNIGHT][square].opening, PIECE_SQUARE[BLACK_KNIGHT][square ^ 56].opening);
        EXPECT_EQ(PIECE_SQUARE[WHITE_KING][square].closing, PIECE_SQUARE[BLACK_KING][square ^ 56].closing);
    }
}

}
pasteque_namespace_end

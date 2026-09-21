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

#ifndef BOARD_H
#define BOARD_H

#include <string>

#include "pasteque.h"
#include "bitboard.h"
#include "move.h"

pasteque_namespace_begin

#define START_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

struct Rewind
{
    unsigned char   m_rights,
                    m_enPassant,
                    m_captured;

    unsigned int    m_halfMoves;
};

class Board
{
public:
    Board();

public:
    bool            setFen(const std::string & fen);
    std::string     fen() const;
    void            setInitial();
    void            clear();

public:
    bitboard        getPieces(unsigned char piece) const { return m_pieces[piece]; }
    bitboard        getAllPieces(unsigned char color) const { return m_allPieces[color]; }
    bitboard        getAllPieces() const { return m_allPieces[WHITE] | m_allPieces[BLACK]; }
    unsigned char   getPiece(int square) const { return m_squares[square]; }

public:
    unsigned char   getSide() const { return m_side; }
    unsigned char   getCastlings() const { return m_rights; }
    unsigned char   getEp() const { return m_enPassant; }
    unsigned int    getFifty() const { return m_halfMoves; }
    unsigned int    getMoveNumber() const { return m_moveNumber; }
    int             getKing(unsigned char color) const { return firstOne(m_pieces[piece_of(KING, color)]); }

public:
    bool            makeMove(Move move, Rewind & undo);
    void            doMove(Move move, Rewind & undo);
    void            unmakeMove(Move move, const Rewind & undo);

    bool            isAttacked(int square, unsigned char side) const;
    bool            isAttackedBy(int square, unsigned char side, bitboard occupied) const;
    bool            inCheck() const { return isAttacked(getKing(m_side), m_side ^ 1); }

public:
    bitboard        getCheckers() const;
    bitboard        getPinned(unsigned char side) const;
    bool            legal(Move move, bitboard pinned, int king) const;

private:
    void            putPiece(int square, unsigned char piece);
    void            removePiece(int square);
    void            movePiece(int from, int to);

private:
    bitboard        m_pieces[16],
                    m_allPieces[2];

    unsigned char   m_squares[64];

    unsigned char   m_side,
                    m_rights,
                    m_enPassant;

    unsigned int    m_halfMoves,
                    m_moveNumber;
};

pasteque_namespace_end
#endif // BOARD_H

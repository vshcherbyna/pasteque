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

#include "moves.h"
#include "attacks.h"

pasteque_namespace_begin

void Moves::generate(const Board & board) {

    m_size = 0;

    auto side = board.getSide();
    auto free = ~board.getAllPieces(side);

    generatePawns(board, side, ~0ULL, 0);

    generatePieces(board, side, KNIGHT, free);
    generatePieces(board, side, BISHOP, free);
    generatePieces(board, side, ROOK, free);
    generatePieces(board, side, QUEEN, free);
    generatePieces(board, side, KING, free);

    generateCastlings(board, side);
}

void Moves::generateLegal(const Board & board) {

    m_size = 0;

    auto side     = board.getSide();
    auto king     = board.getKing(side);
    auto checkers = board.getCheckers();
    auto free     = ~board.getAllPieces(side);

    if (checkers && (checkers & (checkers - 1)))
        generatePieces(board, side, KING, free);
    else {
        auto mask = checkers ? (BETWEEN[king][firstOne(checkers)] | checkers) : ~0ULL;

        generatePawns(board, side, mask, checkers);

        generatePieces(board, side, KNIGHT, free & mask);
        generatePieces(board, side, BISHOP, free & mask);
        generatePieces(board, side, ROOK, free & mask);
        generatePieces(board, side, QUEEN, free & mask);
        generatePieces(board, side, KING, free);

        if (!checkers)
            generateCastlings(board, side);
    }

    auto pinned = board.getPinned(side);
    auto i = 0;

    while (i < m_size) {
        auto from = m_moves[i].getFrom();

        if (((pinned & bit_of(from)) || from == king || m_moves[i].isEnPassant())
            && !board.legal(m_moves[i], pinned, king))
            m_moves[i] = m_moves[--m_size];
        else
            ++i;
    }
}

void Moves::addPromotions(int from, int to, int piece, int capture) {

    auto side = piece_color(piece);

    add(Move(from, to, piece, capture, piece_of(QUEEN,  side)));
    add(Move(from, to, piece, capture, piece_of(ROOK,   side)));
    add(Move(from, to, piece, capture, piece_of(BISHOP, side)));
    add(Move(from, to, piece, capture, piece_of(KNIGHT, side)));
}

void Moves::generatePawns(const Board & board, unsigned char side, bitboard mask, bitboard checkers) {

    auto piece    = piece_of(PAWN, side);
    auto pawns    = board.getPieces(piece);

    if (!pawns)
        return;

    auto empty    = ~board.getAllPieces();
    auto enemies  = board.getAllPieces(side ^ 1);

    auto forward  = (side == WHITE) ? 8 : -8;
    auto lastRank = (side == WHITE) ? rank_mask(7) : rank_mask(0);

    bitboard single, doubled;

    if (side == WHITE) {
        single  = (pawns << 8) & empty;
        doubled = ((single & rank_mask(2)) << 8) & empty;
    }
    else {
        single  = (pawns >> 8) & empty;
        doubled = ((single & rank_mask(5)) >> 8) & empty;
    }

    single  &= mask;
    doubled &= mask;

    while (single) {
        auto to = popFirstOne(single);

        if (bit_of(to) & lastRank)
            addPromotions(to - forward, to, piece, EMPTY);
        else
            add(Move(to - forward, to, piece));
    }

    while (doubled) {
        auto to = popFirstOne(doubled);
        add(Move(to - 2 * forward, to, piece));
    }

    auto capturers = pawns;

    while (capturers) {

        auto from    = popFirstOne(capturers);
        auto attacks = PAWN_ATTACKS[side][from] & enemies & mask;

        while (attacks) {

            auto to = popFirstOne(attacks);

            if (bit_of(to) & lastRank)
                addPromotions(from, to, piece, board.getPiece(to));
            else
                add(Move(from, to, piece, board.getPiece(to)));
        }
    }

    if (board.getEp() != NO_SQUARE) {
        auto ep = board.getEp();

        if (checkers && checkers != bit_of((side == WHITE) ? ep - 8 : ep + 8))
            return;

        auto takers = PAWN_ATTACKS[side ^ 1][ep] & pawns;

        while (takers) {
            auto from = popFirstOne(takers);
            add(Move(from, ep, piece, piece_of(PAWN, side ^ 1), EMPTY, MOVE_ENPASSANT));
        }
    }
}

void Moves::generatePieces(const Board & board, unsigned char side, unsigned char type, bitboard mask) {

    auto piece    = static_cast<unsigned char>(piece_of(type, side));
    auto pieces   = board.getPieces(piece);
    auto occupied = board.getAllPieces();
    auto targets  = mask;

    while (pieces) {
        auto from = popFirstOne(pieces);
        auto attacks = pieceAttacks(piece, from, occupied) & targets;

        while (attacks) {
            auto to = popFirstOne(attacks);
            add(Move(from, to, piece, board.getPiece(to)));
        }
    }
}

void Moves::generateCastlings(const Board & board, unsigned char side) {

    auto castlings = board.getCastlings();

    if (!castlings)
        return;

    auto occupied = board.getAllPieces();
    auto king     = piece_of(KING, side);

    if (side == WHITE) {
        if ((castlings & WHITE_SHORT) && !(occupied & (bit_of(F1) | bit_of(G1))))
            add(Move(E1, G1, king, EMPTY, EMPTY, MOVE_CASTLING));

        if ((castlings & WHITE_LONG) && !(occupied & (bit_of(B1) | bit_of(C1) | bit_of(D1))))
            add(Move(E1, C1, king, EMPTY, EMPTY, MOVE_CASTLING));
    }
    else {
        if ((castlings & BLACK_SHORT) && !(occupied & (bit_of(F8) | bit_of(G8))))
            add(Move(E8, G8, king, EMPTY, EMPTY, MOVE_CASTLING));

        if ((castlings & BLACK_LONG) && !(occupied & (bit_of(B8) | bit_of(C8) | bit_of(D8))))
            add(Move(E8, C8, king, EMPTY, EMPTY, MOVE_CASTLING));
    }
}

pasteque_namespace_end

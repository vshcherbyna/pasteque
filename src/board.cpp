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

#include <cstring>

#include "board.h"
#include "attacks.h"

pasteque_namespace_begin

static const char PIECE_CHARS[] = ".NPK.BRQ.npk.brq";

static unsigned char charToPiece(char ch) {
    switch (ch) {
    case 'N': return WHITE_KNIGHT;
    case 'P': return WHITE_PAWN;
    case 'K': return WHITE_KING;
    case 'B': return WHITE_BISHOP;
    case 'R': return WHITE_ROOK;
    case 'Q': return WHITE_QUEEN;
    case 'n': return BLACK_KNIGHT;
    case 'p': return BLACK_PAWN;
    case 'k': return BLACK_KING;
    case 'b': return BLACK_BISHOP;
    case 'r': return BLACK_ROOK;
    case 'q': return BLACK_QUEEN;
    }

    return EMPTY;
}

static const char * skipSpaces(const char * p) {
    while (*p == ' ')
        ++p;

    return p;
}

static const char * readNumber(const char * p, unsigned int & value) {
    if (*p < '0' || *p > '9')
        return p;

    unsigned int number = 0;

    while (*p >= '0' && *p <= '9')
        number = number * 10 + static_cast<unsigned int>(*p++ - '0');

    value = number;

    return p;
}

static unsigned char CASTLING_MASK[64];

static void initCastlingMask() {
    static auto initialised = false;

    if (initialised)
        return;

    initialised = true;

    for (auto square = 0; square < 64; ++square)
        CASTLING_MASK[square] = 0xff;

    CASTLING_MASK[E1] = static_cast<unsigned char>(~(WHITE_SHORT | WHITE_LONG));
    CASTLING_MASK[A1] = static_cast<unsigned char>(~WHITE_LONG);
    CASTLING_MASK[H1] = static_cast<unsigned char>(~WHITE_SHORT);

    CASTLING_MASK[E8] = static_cast<unsigned char>(~(BLACK_SHORT | BLACK_LONG));
    CASTLING_MASK[A8] = static_cast<unsigned char>(~BLACK_LONG);
    CASTLING_MASK[H8] = static_cast<unsigned char>(~BLACK_SHORT);
}

Board::Board() {
    initAttacks();
    initCastlingMask();

    setInitial();
}

void Board::clear() {
    std::memset(m_pieces, 0, sizeof(m_pieces));
    std::memset(m_allPieces, 0, sizeof(m_allPieces));
    std::memset(m_squares, EMPTY, sizeof(m_squares));

    m_side      = WHITE;
    m_rights    = 0;
    m_enPassant = NO_SQUARE;

    m_halfMoves  = 0;
    m_moveNumber = 1;
}

void Board::setInitial() {
    setFen(START_POSITION);
}

void Board::putPiece(int square, unsigned char piece) {
    bit_set(m_pieces[piece], square);
    bit_set(m_allPieces[piece_color(piece)], square);

    m_squares[square] = piece;
}

void Board::removePiece(int square) {
    auto piece = m_squares[square];

    bit_unset(m_pieces[piece], square);
    bit_unset(m_allPieces[piece_color(piece)], square);

    m_squares[square] = EMPTY;
}

void Board::movePiece(int from, int to) {
    auto piece = m_squares[from];
    auto mask  = bit_of(from) | bit_of(to);

    m_pieces[piece] ^= mask;
    m_allPieces[piece_color(piece)] ^= mask;

    m_squares[from] = EMPTY;
    m_squares[to]   = piece;
}

bool Board::setFen(const std::string & fen) {
    clear();

    auto p = skipSpaces(fen.c_str());
    auto square = static_cast<int>(A8);

    for (; *p && *p != ' '; ++p) {

        if (*p >= '1' && *p <= '8') {
            square += *p - '0';
            continue;
        }

        if (*p == '/') {
            square -= 16;
            continue;
        }

        auto piece = charToPiece(*p);

        if (piece == EMPTY || square < A1 || square > H8)
            return false;

        putPiece(square++, piece);
    }

    if (square != H1 + 1)
        return false;

    p = skipSpaces(p);

    if (*p == 'w')
        m_side = WHITE;
    else if (*p == 'b')
        m_side = BLACK;
    else
        return false;

    if (*++p && *p != ' ')
        return false;

    p = skipSpaces(p);

    if (!*p)
        return false;

    for (; *p && *p != ' '; ++p) {
        switch (*p) {
        case 'K': m_rights |= WHITE_SHORT;  break;
        case 'Q': m_rights |= WHITE_LONG; break;
        case 'k': m_rights |= BLACK_SHORT;  break;
        case 'q': m_rights |= BLACK_LONG; break;
        case '-': break;
        default: return false;
        }
    }

    p = skipSpaces(p);

    if (*p != '-') {
        if (p[0] < 'a' || p[0] > 'h' || p[1] < '1' || p[1] > '8')
            return false;

        m_enPassant = static_cast<unsigned char>(square_of(p[0] - 'a', p[1] - '1'));
        p += 2;
    }
    else
        ++p;

    if (*p && *p != ' ')
        return false;

    p = readNumber(skipSpaces(p), m_halfMoves);
    readNumber(skipSpaces(p), m_moveNumber);

    return true;
}

std::string Board::fen() const {
    std::string fen;

    fen.reserve(90);

    for (auto rank = 7; rank >= 0; --rank) {
        auto empty = 0;

        for (auto file = 0; file < 8; ++file) {
            auto piece = m_squares[square_of(file, rank)];

            if (piece == EMPTY) {
                ++empty;
                continue;
            }

            if (empty) {
                fen += static_cast<char>('0' + empty);
                empty = 0;
            }

            fen += PIECE_CHARS[piece];
        }

        if (empty)
            fen += static_cast<char>('0' + empty);

        if (rank)
            fen += '/';
    }

    fen += (m_side == WHITE) ? " w " : " b ";

    if (m_rights) {
        if (m_rights & WHITE_SHORT) fen += 'K';
        if (m_rights & WHITE_LONG)  fen += 'Q';
        if (m_rights & BLACK_SHORT) fen += 'k';
        if (m_rights & BLACK_LONG)  fen += 'q';
    }
    else
        fen += '-';

    if (m_enPassant == NO_SQUARE)
        fen += " -";
    else {
        fen += ' ';
        fen += static_cast<char>('a' + square_file(m_enPassant));
        fen += static_cast<char>('1' + square_rank(m_enPassant));
    }

    fen += ' ' + std::to_string(m_halfMoves);
    fen += ' ' + std::to_string(m_moveNumber);

    return fen;
}

bool Board::isAttacked(int square, unsigned char side) const {
    return isAttackedBy(square, side, getAllPieces());
}

bool Board::isAttackedBy(int square, unsigned char side, bitboard occupied) const {
    if (PAWN_ATTACKS[side ^ 1][square] & m_pieces[piece_of(PAWN, side)])
        return true;

    if (KNIGHT_ATTACKS[square] & m_pieces[piece_of(KNIGHT, side)])
        return true;

    if (KING_ATTACKS[square] & m_pieces[piece_of(KING, side)])
        return true;

    auto queens = m_pieces[piece_of(QUEEN, side)];

    if (bishopAttacks(square, occupied) & (m_pieces[piece_of(BISHOP, side)] | queens))
        return true;

    if (rookAttacks(square, occupied) & (m_pieces[piece_of(ROOK, side)] | queens))
        return true;

    return false;
}

bool Board::makeMove(Move move, Rewind & undo) {
    auto side     = m_side;
    auto opponent = static_cast<unsigned char>(side ^ 1);
    auto from     = move.getFrom();
    auto to       = move.getTo();

    //
    //  A king may not castle out of check, nor through an attacked square
    //

    if (move.isCastling() && (isAttacked(from, opponent) || isAttacked((from + to) / 2, opponent)))
        return false;

    doMove(move, undo);

    if (isAttacked(getKing(side), opponent)) {
        unmakeMove(move, undo);
        return false;
    }

    return true;
}

void Board::doMove(Move move, Rewind & undo) {
    auto from     = move.getFrom();
    auto to       = move.getTo();
    auto piece    = move.getPiece();
    auto side     = m_side;
    auto opponent = static_cast<unsigned char>(side ^ 1);

    undo.m_rights    = m_rights;
    undo.m_enPassant = m_enPassant;
    undo.m_halfMoves = m_halfMoves;
    undo.m_captured  = EMPTY;

    m_enPassant = NO_SQUARE;
    ++m_halfMoves;

    if (move.isEnPassant()) {
        auto captured = square_of(square_file(to), square_rank(from));

        undo.m_captured = m_squares[captured];

        removePiece(captured);
        movePiece(from, to);

        m_halfMoves = 0;
    }
    else
    {
        if (m_squares[to] != EMPTY) {
            undo.m_captured = m_squares[to];

            removePiece(to);

            m_halfMoves = 0;
        }

        movePiece(from, to);

        if ((piece & 7) == PAWN) {
            m_halfMoves = 0;

            if (move.getPromotion()) {
                removePiece(to);
                putPiece(to, static_cast<unsigned char>(move.getPromotion()));
            }
            else if ((from ^ to) == 16)
                m_enPassant = static_cast<unsigned char>((from + to) / 2);
        }
        else if (move.isCastling()) {
            auto rookFrom = (to > from) ? (to + 1) : (to - 2);
            auto rookTo   = (to > from) ? (to - 1) : (to + 1);

            movePiece(rookFrom, rookTo);
        }
    }

    m_rights &= CASTLING_MASK[from] & CASTLING_MASK[to];
    m_side = opponent;

    if (side == BLACK)
        ++m_moveNumber;
}

void Board::unmakeMove(Move move, const Rewind & undo) {

    auto from  = move.getFrom();
    auto to    = move.getTo();
    auto piece = move.getPiece();

    m_side ^= 1;

    if (m_side == BLACK)
        --m_moveNumber;

    if (move.getPromotion()) {
        removePiece(to);
        putPiece(to, static_cast<unsigned char>(piece));
    }

    movePiece(to, from);

    if (move.isEnPassant())
        putPiece(square_of(square_file(to), square_rank(from)), undo.m_captured);
    else if (undo.m_captured != EMPTY)
        putPiece(to, undo.m_captured);

    if (move.isCastling()) {
        auto rookFrom = (to > from) ? (to + 1) : (to - 2);
        auto rookTo   = (to > from) ? (to - 1) : (to + 1);

        movePiece(rookTo, rookFrom);
    }

    m_rights    = undo.m_rights;
    m_enPassant = undo.m_enPassant;
    m_halfMoves = undo.m_halfMoves;
}

bitboard Board::getCheckers() const {
    auto king     = getKing(m_side);
    auto opponent = static_cast<unsigned char>(m_side ^ 1);
    auto occupied = getAllPieces();
    auto queens   = m_pieces[piece_of(QUEEN, opponent)];

    return (PAWN_ATTACKS[m_side][king] & m_pieces[piece_of(PAWN, opponent)])
               | (KNIGHT_ATTACKS[king] & m_pieces[piece_of(KNIGHT, opponent)])
               | (bishopAttacks(king, occupied) & (m_pieces[piece_of(BISHOP, opponent)] | queens))
               | (rookAttacks(king, occupied) & (m_pieces[piece_of(ROOK, opponent)] | queens));
}

bitboard Board::getPinned(unsigned char side) const {
    auto king     = getKing(side);
    auto opponent = static_cast<unsigned char>(side ^ 1);
    auto occupied = getAllPieces();
    auto queens   = m_pieces[piece_of(QUEEN, opponent)];

    auto snipers = (rookAttacks(king, 0) & (m_pieces[piece_of(ROOK, opponent)] | queens))
                 | (bishopAttacks(king, 0) & (m_pieces[piece_of(BISHOP, opponent)] | queens));

    bitboard pinned = 0;

    while (snipers) {
        auto sniper   = popFirstOne(snipers);
        auto blockers = BETWEEN[king][sniper] & occupied;

        if (blockers && !(blockers & (blockers - 1))) 
            pinned |= blockers & m_allPieces[side];
    }

    return pinned;
}

bool Board::legal(Move move, bitboard pinned, int king) const {
    auto from     = move.getFrom();
    auto to       = move.getTo();
    auto opponent = static_cast<unsigned char>(m_side ^ 1);

    if (move.isEnPassant()) {
        auto captured = square_of(square_file(to), square_rank(from));
        auto occupied = (getAllPieces() ^ bit_of(from) ^ bit_of(captured)) | bit_of(to);
        auto queens   = m_pieces[piece_of(QUEEN, opponent)];

        return !(rookAttacks(king, occupied) & (m_pieces[piece_of(ROOK, opponent)] | queens))
            && !(bishopAttacks(king, occupied) & (m_pieces[piece_of(BISHOP, opponent)] | queens));
    }

    if (from == king) {
        if (move.isCastling()) {
            auto middle   = (from + to) / 2;
            auto rookFrom = (to > from) ? (to + 1) : (to - 2);
            auto occupied = (getAllPieces() ^ bit_of(from) ^ bit_of(rookFrom)) | bit_of(to) | bit_of(middle);

            return !isAttacked(middle, opponent) && !isAttackedBy(to, opponent, occupied);
        }

        return !isAttackedBy(to, opponent, getAllPieces() ^ bit_of(from));
    }

    return !(pinned & bit_of(from)) || (LINE[king][from] & bit_of(to));
}

pasteque_namespace_end

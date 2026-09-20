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

#include "search.h"
#include "judge.h"

#include <chrono>

pasteque_namespace_begin

static const int ORDER_VALUES[8] = { 0, 3, 1, 0, 0, 3, 5, 9 };

Search::Search() : m_nodes{0}, m_score{0}, m_watcher{nullptr} {
    Judge::init();
}

void Search::order(Moves & moves) {

    int scores[MOVE_LIMIT];

    for (auto i = 0; i < moves.size(); ++i) {
        auto move  = moves[i];
        auto score = 0;

        if (move.getCapture() != EMPTY)
            score = 1000 + 10 * ORDER_VALUES[move.getCapture() & 7] - ORDER_VALUES[move.getPiece() & 7];

        if (move.getPromotion())
            score += 2000 + ORDER_VALUES[move.getPromotion() & 7];

        scores[i] = score;
    }

    for (auto i = 1; i < moves.size(); ++i) {
        auto move  = moves[i];
        auto score = scores[i];
        auto j     = i - 1;

        while (j >= 0 && scores[j] < score) {
            moves[j + 1]  = moves[j];
            scores[j + 1] = scores[j];
            --j;
        }

        moves[j + 1]  = move;
        scores[j + 1] = score;
    }
}

int Search::quiescence(Board & board, int alpha, int beta, int ply) {

    ++m_nodes;

    if (ply >= PLY_LIMIT)
        return Judge::evaluate(board);

    auto checked = board.getCheckers() != 0;
    auto best    = -static_cast<int>(HUGE_SCORE);

    if (!checked) {
        best = Judge::evaluate(board);

        if (best >= beta)
            return best;

        if (best > alpha)
            alpha = best;
    }

    Moves moves;
    moves.generateLegal(board);

    if (!moves.size())
        return checked ? -MATE_SCORE + ply : EVEN_SCORE;

    order(moves);

    for (auto i = 0; i < moves.size(); ++i) {
        auto move = moves[i];

        if (!checked && move.getCapture() == EMPTY && !move.getPromotion())
            continue;

        Rewind undo;

        board.doMove(move, undo);
        auto score = -quiescence(board, -beta, -alpha, ply + 1);
        board.unmakeMove(move, undo);

        if (score <= best)
            continue;

        best = score;

        if (score <= alpha)
            continue;

        alpha = score;

        if (alpha >= beta)
            break;
    }

    return best;
}

int Search::alphaBeta(Board & board, int alpha, int beta, int depth, int ply) {

    if (depth <= 0)
        return quiescence(board, alpha, beta, ply);

    ++m_nodes;

    if (ply > 0 && board.getFifty() >= 100)
        return EVEN_SCORE;

    Moves moves;
    moves.generateLegal(board);

    if (!moves.size())
        return board.getCheckers() ? -MATE_SCORE + ply : EVEN_SCORE;

    order(moves);

    auto best = -static_cast<int>(HUGE_SCORE);

    for (auto i = 0; i < moves.size(); ++i) {

        Rewind undo;

        board.doMove(moves[i], undo);
        auto score = -alphaBeta(board, -beta, -alpha, depth - 1, ply + 1);
        board.unmakeMove(moves[i], undo);

        if (score <= best)
            continue;

        best = score;

        if (score <= alpha)
            continue;

        alpha = score;

        if (alpha >= beta)
            break;
    }

    return best;
}

Move Search::bestMove(Board & board, int depth) {

    m_nodes = 0;
    m_score = 0;

    Moves moves;
    moves.generateLegal(board);

    if (!moves.size())
        return Move();

    order(moves);

    auto best    = moves[0];
    auto started = std::chrono::steady_clock::now();

    for (auto iteration = 1; iteration <= depth; ++iteration) {
        auto alpha = -static_cast<int>(HUGE_SCORE);
        auto chosen = 0;

        for (auto i = 0; i < moves.size(); ++i) {
            Rewind undo;

            board.doMove(moves[i], undo);
            auto score = -alphaBeta(board, -HUGE_SCORE, -alpha, iteration - 1, 1);
            board.unmakeMove(moves[i], undo);

            if (score > alpha) {
                alpha  = score;
                chosen = i;
            }
        }

        best    = moves[chosen];
        m_score = alpha;

        for (auto i = chosen; i > 0; --i)
            moves[i] = moves[i - 1];

        moves[0] = best;

        if (m_watcher) {
            auto elapsed = std::chrono::steady_clock::now() - started;
            auto msec    = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

            m_watcher(iteration, m_score, m_nodes, static_cast<unsigned long long>(msec), best);
        }
    }

    return best;
}

pasteque_namespace_end

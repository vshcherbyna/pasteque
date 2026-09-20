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

#include "search.h"
#include "judge.h"

#include <chrono>

pasteque_namespace_begin

static const int ORDER_VALUES[8] = { 0, 1, 7, 6, 0, 10, 15, 9 };

Search::Search() : m_nodes{0}, m_score{0}, m_watcher{nullptr}, m_timed{false}, m_aborted{false} {
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

void Search::pollClock() {

    if (m_timed && std::chrono::steady_clock::now() >= m_deadline)
        m_aborted = true;
}

static unsigned long long spentMs(Instant started) {

    auto span = std::chrono::steady_clock::now() - started;

    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::milliseconds>(span).count());
}

int Search::quiescence(Board & board, int alpha, int beta, int ply) {

    if ((++m_nodes & POLL_MASK) == 0)
        pollClock();

    if (m_aborted)
        return 0;

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

    if ((++m_nodes & POLL_MASK) == 0)
        pollClock();

    if (m_aborted)
        return 0;

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

    m_timed   = false;
    m_aborted = false;

    return deepen(board, depth, std::chrono::steady_clock::now(), 0);
}

Move Search::bestMove(Board & board, const Clock & clock) {

    auto started = std::chrono::steady_clock::now();

    m_timed   = !clock.isEndless();
    m_aborted = false;

    if (m_timed)
        m_deadline = started + std::chrono::milliseconds(clock.getHard());

    return deepen(board, clock.getDepth(), started, m_timed ? clock.getSoft() : 0);
}

//
//  An iteration abandoned part way through has searched only some of the root moves, so
//  its winner is not comparable with the rest. The move from the last finished iteration
//  is kept instead
//

Move Search::deepen(Board & board, int depth, Instant started, unsigned int soft) {

    m_nodes = 0;
    m_score = 0;

    Moves moves;
    moves.generateLegal(board);

    if (!moves.size())
        return Move();

    order(moves);

    auto best = moves[0];

    for (auto iteration = 1; iteration <= depth; ++iteration) {
        auto alpha  = -static_cast<int>(HUGE_SCORE);
        auto chosen = 0;

        for (auto i = 0; i < moves.size(); ++i) {
            Rewind undo;

            board.doMove(moves[i], undo);
            auto score = -alphaBeta(board, -HUGE_SCORE, -alpha, iteration - 1, 1);
            board.unmakeMove(moves[i], undo);

            if (m_aborted)
                break;

            if (score > alpha) {
                alpha  = score;
                chosen = i;
            }
        }

        if (m_aborted)
            break;

        best    = moves[chosen];
        m_score = alpha;

        for (auto i = chosen; i > 0; --i)
            moves[i] = moves[i - 1];

        moves[0] = best;

        auto spent = spentMs(started);

        if (m_watcher)
            m_watcher(iteration, m_score, m_nodes, spent, best);

        //
        //  The next iteration costs several times this one, so starting one that the soft
        //  budget cannot cover only risks being cut off with nothing to show for it
        //

        if (soft && spent * 2 >= soft)
            break;
    }

    return best;
}

pasteque_namespace_end

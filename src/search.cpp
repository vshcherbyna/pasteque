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

//
//  The fitted piece values. The king is not among them - it cancels between the sides, so the
//  fit cannot see it - and it takes the dearest attacker's value, because any other attacker is
//  preferable when one is available
//
//  ORDER_VICTIM has to exceed the spread between the dearest and the cheapest attacker divided
//  by the smallest gap between two neighbouring victims, or a cheap piece taking a lesser victim
//  outranks an expensive one taking a greater. The offsets keep the three classes apart whatever
//  the values grow to
//

static const int ORDER_VALUES[8] = { 0, 199, 100, 627, 0, 226, 335, 627 };

Search::Search() : m_nodes{0}, m_quota{0}, m_score{0}, m_watcher{nullptr}, m_timed{false}, m_aborted{false} {
    Judge::init();
    setHash(HASH_DEFAULT);
}

void Search::setHash(int megabytes) {

    if (megabytes < static_cast<int>(HASH_LEAST))
        megabytes = HASH_LEAST;

    if (megabytes > static_cast<int>(HASH_MOST))
        megabytes = HASH_MOST;

    auto room  = static_cast<size_t>(megabytes) * 1024 * 1024 / sizeof(HashEntry);
    auto slots = static_cast<size_t>(1);

    while (slots * 2 <= room)
        slots *= 2;

    m_slots = slots;
    m_age   = 1;

    m_hash = std::vector<HashEntry>(slots);
}

void Search::order(Moves & moves, int ply, Move favoured) {

    int scores[MOVE_LIMIT];

    auto remembered = ply >= 0 && ply < static_cast<int>(PLY_LIMIT);

    for (auto i = 0; i < moves.size(); ++i) {
        auto move  = moves[i];
        auto score = 0;

        if (static_cast<int>(favoured) && static_cast<int>(move) == static_cast<int>(favoured))
            score = ORDER_HASH;
        else if (move.getCapture() != EMPTY)
            score = ORDER_CAPTURE + ORDER_VICTIM * ORDER_VALUES[move.getCapture() & 7] - ORDER_VALUES[move.getPiece() & 7];
        else if (remembered && static_cast<int>(move) == static_cast<int>(m_killers[ply][0]))
            score = ORDER_KILLER + 1;
        else if (remembered && static_cast<int>(move) == static_cast<int>(m_killers[ply][1]))
            score = ORDER_KILLER;
        else
            score = m_merit[move.getPiece() & 15][move.getTo()];

        if (move.getPromotion())
            score += ORDER_PROMOTION + ORDER_VALUES[move.getPromotion() & 7];

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

static unsigned int signet(stamp key, int fifty) {

    auto plain = static_cast<unsigned int>(key >> 32);

    if (fifty < static_cast<int>(HASH_HORIZON))
        return plain;

    return plain ^ (static_cast<unsigned int>(fifty) * 2654435769u);
}

static int lookahead(int score, int depth) {

    if (score > static_cast<int>(MATE_SCORE) - static_cast<int>(PLY_LIMIT))
        return static_cast<int>(MATE_SCORE) - score;

    if (score < -static_cast<int>(MATE_SCORE) + static_cast<int>(PLY_LIMIT))
        return static_cast<int>(MATE_SCORE) + score;

    return depth;
}

int Search::reduce(Move move, int depth, int played, bool checked, bool checking) {

    if (checked || checking)
        return 0;

    if (depth < static_cast<int>(REDUCE_DEPTH) || played < static_cast<int>(REDUCE_MOVES))
        return 0;

    if (move.getCapture() != EMPTY || move.getPromotion())
        return 0;

    auto reduction = 1;

    if (depth >= static_cast<int>(REDUCE_MORE_DEPTH) && played >= static_cast<int>(REDUCE_MORE_MOVES))
        reduction = 2;

    return reduction;
}

bool Search::probe(stamp key, int fifty, int depth, int ply, int alpha, int beta, int & score, Move & favoured) {

    auto & slot = m_hash[key & (m_slots - 1)];

    if (slot.age != m_age || slot.check != signet(key, fifty))
        return false;

    favoured = Move(static_cast<unsigned int>(slot.move));

    if (slot.depth < depth)
        return false;

    auto kept = static_cast<int>(slot.score);

    if (fifty + lookahead(kept, slot.depth) >= 100)
        return false;

    if (kept > static_cast<int>(MATE_SCORE) - static_cast<int>(PLY_LIMIT))
        kept -= ply;
    else if (kept < -static_cast<int>(MATE_SCORE) + static_cast<int>(PLY_LIMIT))
        kept += ply;

    if (slot.bound == HASH_EXACT
        || (slot.bound == HASH_LOWER && kept >= beta)
        || (slot.bound == HASH_UPPER && kept <= alpha)) {
        score = kept;
        return true;
    }

    return false;
}

void Search::store(stamp key, int fifty, int depth, int ply, int score, int bound, Move move) {

    if (score > static_cast<int>(MATE_SCORE) - static_cast<int>(PLY_LIMIT))
        score += ply;
    else if (score < -static_cast<int>(MATE_SCORE) + static_cast<int>(PLY_LIMIT))
        score -= ply;

    if (fifty + lookahead(score, depth) >= 100)
        return;

    auto & slot = m_hash[key & (m_slots - 1)];

    if (slot.age == m_age && slot.depth > depth)
        return;

    slot.check = signet(key, fifty);
    slot.age   = m_age;
    slot.move  = static_cast<int>(move);
    slot.score = static_cast<short>(score);
    slot.depth = static_cast<signed char>(depth);
    slot.bound = static_cast<unsigned char>(bound);
}

void Search::reward(Move move, int depth, int ply) {

    if (move.getCapture() != EMPTY || move.getPromotion())
        return;

    auto & earned = m_merit[move.getPiece() & 15][move.getTo()];

    earned += depth * depth;

    if (earned > static_cast<int>(ORDER_MERIT))
        earned = ORDER_MERIT;

    if (ply < 0 || ply >= static_cast<int>(PLY_LIMIT))
        return;

    if (static_cast<int>(m_killers[ply][0]) == static_cast<int>(move))
        return;

    m_killers[ply][1] = m_killers[ply][0];
    m_killers[ply][0] = move;
}

void Search::pollClock() {

    if (m_quota && m_nodes >= m_quota)
        m_aborted = true;

    if (m_timed && std::chrono::steady_clock::now() >= m_deadline)
        m_aborted = true;
}

static unsigned long long spentMs(Instant started) {

    auto span = std::chrono::steady_clock::now() - started;

    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::milliseconds>(span).count());
}

int Search::quiescence(Board & board, int alpha, int beta, int ply) {

    if ((++m_nodes & POLL_MASK) == 0 || m_nodes == m_quota)
        pollClock();

    if (m_aborted)
        return 0;

    if (ply > 0 && board.recurred(ply))
        return EVEN_SCORE;

    if (ply >= PLY_LIMIT)
        return Judge::evaluate(board);

    auto checked = board.getCheckers() != 0;
    auto expired = ply > 0 && board.getFifty() >= 100;

    if (expired && !checked)
        return EVEN_SCORE;

    auto best = -static_cast<int>(HUGE_SCORE);

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

    if (expired)
        return EVEN_SCORE;

    order(moves, ply, Move());

    for (auto i = 0; i < moves.size(); ++i) {
        auto move = moves[i];

        if (!checked && move.getCapture() == EMPTY && !move.getPromotion())
            continue;

        Rewind undo;

        board.doMove(move, undo);
        auto score = -quiescence(board, -beta, -alpha, ply + 1);
        board.unmakeMove(move, undo);

        if (m_aborted)
            return 0;

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

    if (ply > 0 && board.recurred(ply))
        return EVEN_SCORE;

    if (depth <= 0)
        return quiescence(board, alpha, beta, ply);

    if ((++m_nodes & POLL_MASK) == 0 || m_nodes == m_quota)
        pollClock();

    if (m_aborted)
        return 0;

    auto opening = alpha;
    auto kept    = 0;

    Move favoured;

    if (probe(board.getStamp(), board.getFifty(), depth, ply, alpha, beta, kept, favoured))
        return kept;

    Moves moves;
    moves.generateLegal(board);

    if (!moves.size())
        return board.getCheckers() ? -MATE_SCORE + ply : EVEN_SCORE;

    if (ply > 0 && board.getFifty() >= 100)
        return EVEN_SCORE;

    order(moves, ply, favoured);

    auto best    = -static_cast<int>(HUGE_SCORE);
    auto checked = board.getCheckers() != 0;

    Move chosen;

    for (auto i = 0; i < moves.size(); ++i) {

        Rewind undo;

        board.doMove(moves[i], undo);

        auto reduction = reduce(moves[i], depth, i, checked, board.getCheckers() != 0);
        auto score     = 0;

        if (reduction) {
            score = -alphaBeta(board, -alpha - 1, -alpha, depth - 1 - reduction, ply + 1);

            if (!m_aborted && score > alpha)
                score = -alphaBeta(board, -beta, -alpha, depth - 1, ply + 1);
        }
        else
            score = -alphaBeta(board, -beta, -alpha, depth - 1, ply + 1);

        board.unmakeMove(moves[i], undo);

        if (m_aborted)
            return 0;

        if (score <= best)
            continue;

        best   = score;
        chosen = moves[i];

        if (score <= alpha)
            continue;

        alpha = score;

        if (alpha >= beta) {
            reward(moves[i], depth, ply);
            break;
        }
    }

    auto bound = (best >= beta) ? HASH_LOWER : ((best > opening) ? HASH_EXACT : HASH_UPPER);

    store(board.getStamp(), board.getFifty(), depth, ply, best, bound, chosen);

    return best;
}

Move Search::bestMove(Board & board, int depth) {

    m_timed   = false;
    m_quota   = 0;
    m_aborted = false;

    return deepen(board, depth, std::chrono::steady_clock::now(), 0);
}

Move Search::bestMove(Board & board, const Clock & clock) {

    auto started = std::chrono::steady_clock::now();

    m_timed   = !clock.isEndless();
    m_quota   = clock.getNodes();
    m_aborted = false;

    if (m_timed)
        m_deadline = started + std::chrono::milliseconds(clock.getHard());

    return deepen(board, clock.getDepth(), started, m_timed ? clock.getSoft() : 0);
}

Move Search::deepen(Board & board, int depth, Instant started, unsigned int soft) {

    m_nodes = 0;
    m_score = 0;

    if (++m_age == 0)
        m_age = 1;

    for (auto ply = 0; ply < static_cast<int>(PLY_LIMIT); ++ply) {
        m_killers[ply][0] = Move();
        m_killers[ply][1] = Move();
    }

    for (auto piece = 0; piece < 16; ++piece)
        for (auto square = 0; square < 64; ++square)
            m_merit[piece][square] = 0;

    Moves moves;
    moves.generateLegal(board);

    if (!moves.size())
        return Move();

    order(moves, 0, Move());

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

        if (soft && spent * 2 >= soft)
            break;
    }

    return best;
}

pasteque_namespace_end

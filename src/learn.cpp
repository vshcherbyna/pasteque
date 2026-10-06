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

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <atomic>
#include <map>
#include <string>
#include <thread>

#include "learn.h"
#include "openings.h"
#include "board.h"
#include "moves.h"
#include "search.h"
#include "clock.h"
#include "judge.h"

pasteque_namespace_begin

typedef std::array<int, 5> Census;

struct Harvest
{
    long long   score,
                seen;
};

struct Sample
{
    Census          census;
    unsigned char   side;
};

static const unsigned char LEARN_TYPES[5] = { PAWN, KNIGHT, BISHOP, ROOK, QUEEN };

static const char * const  LEARN_NAMES[5] = { "pawn", "knight", "bishop", "rook", "queen" };

static bool starved(const Board & board) {

    auto mating = board.getPieces(WHITE_PAWN)  | board.getPieces(BLACK_PAWN)
                | board.getPieces(WHITE_ROOK)  | board.getPieces(BLACK_ROOK)
                | board.getPieces(WHITE_QUEEN) | board.getPieces(BLACK_QUEEN);

    if (mating)
        return false;

    auto minors = popCount(board.getPieces(WHITE_KNIGHT) | board.getPieces(WHITE_BISHOP)
                         | board.getPieces(BLACK_KNIGHT) | board.getPieces(BLACK_BISHOP));

    return minors <= 1;
}

static bool concluded(const Board & board, int & result) {

    Moves moves;
    moves.generateLegal(board);

    if (!moves.size()) {
        result = board.getCheckers() ? ((board.getSide() == WHITE) ? -1 : 1) : 0;
        return true;
    }

    if (board.getFifty() >= 100 || board.recurred(0) || starved(board)) {
        result = 0;
        return true;
    }

    return false;
}

static Census census(const Board & board) {

    Census counts;

    auto side     = board.getSide();
    auto opponent = static_cast<unsigned char>(side ^ 1);

    for (auto k = 0; k < 5; ++k)
        counts[k] = popCount(board.getPieces(piece_of(LEARN_TYPES[k], side)))
                  - popCount(board.getPieces(piece_of(LEARN_TYPES[k], opponent)));

    return counts;
}

static void playOut(const std::string & opening, const Clock & clock, Search & searcher,
                    std::map<Census, Harvest> & basket) {

    Board board;

    if (!board.setFen(opening))
        return;

    std::vector<Sample> samples;

    auto result = 0;
    auto ended  = false;

    for (auto ply = 0; ply < static_cast<int>(LEARN_LIMIT); ++ply) {
        if (concluded(board, result)) {
            ended = true;
            break;
        }

        auto best = searcher.bestMove(board, clock);

        if (static_cast<int>(best) == 0)
            break;

        if (!board.getCheckers() && best.getCapture() == EMPTY && !best.getPromotion() && !starved(board))
            samples.push_back({ census(board), board.getSide() });

        Rewind undo;

        board.doMove(best, undo);
    }

    if (!ended)
        return;

    if (samples.size() > static_cast<size_t>(LEARN_KEEP)) {
        std::vector<Sample> thinned;

        auto stride = static_cast<double>(samples.size()) / LEARN_KEEP;

        for (auto i = 0; i < static_cast<int>(LEARN_KEEP); ++i)
            thinned.push_back(samples[static_cast<size_t>(i * stride)]);

        samples.swap(thinned);
    }

    for (const auto & sample : samples) {
        auto   mine = (sample.side == WHITE) ? result : -result;
        auto & cell = basket[sample.census];

        cell.score += mine + 1;
        cell.seen  += 1;
    }
}

void learnRows(std::vector<LearnRow> & rows, int games, int nodes, unsigned long long seed, int threads) {

    if (games < 1)
        games = LEARN_GAMES;

    if (nodes < 1)
        nodes = LEARN_NODES;

    if (threads < 1) {
        threads = static_cast<int>(std::thread::hardware_concurrency());

        if (threads < 1)
            threads = 1;
    }

    if (threads > static_cast<int>(LEARN_THREADS))
        threads = LEARN_THREADS;

    std::vector<std::string> openings;

    openingPositions(openings, games, OPENINGS_PLIES, seed);

    //
    //  The one-time tables guard themselves with a plain bool, which is not safe to race on,
    //  so every one of them has to be built before the first thread starts. Generating the
    //  openings has already built the board's; the evaluation's is asked for here
    //

    Judge::init();

    //
    //  A game depends on nothing but its opening, and the counts are only ever summed, so the
    //  rows come out identical whatever the thread count. Only the wall clock changes
    //

    std::vector<std::map<Census, Harvest>> shares(static_cast<size_t>(threads));

    std::atomic<size_t> next{0},
                        done{0};

    auto note = (games >= 20) ? static_cast<size_t>(games / 20) : static_cast<size_t>(games);

    auto labour = [&](std::map<Census, Harvest> & basket) {
        Clock  clock({ "go", "nodes", std::to_string(nodes) }, WHITE);
        Search searcher;

        for (;;) {
            auto index = next++;

            if (index >= openings.size())
                return;

            playOut(openings[index], clock, searcher, basket);

            auto far = ++done;

            if (note && (far % note) == 0)
                std::cerr << ("learn: " + std::to_string(far) + " of "
                              + std::to_string(openings.size()) + " games\n");
        }
    };

    std::vector<std::thread> hands;

    for (auto & basket : shares)
        hands.push_back(std::thread(labour, std::ref(basket)));

    for (auto & hand : hands)
        hand.join();

    std::map<Census, Harvest> tally;

    for (const auto & basket : shares)
        for (const auto & entry : basket) {
            auto & cell = tally[entry.first];

            cell.score += entry.second.score;
            cell.seen  += entry.second.seen;
        }

    rows.clear();

    for (const auto & entry : tally) {
        LearnRow row;

        for (auto k = 0; k < 5; ++k)
            row.material[k] = entry.first[k];

        row.score = entry.second.score;
        row.seen  = entry.second.seen;

        rows.push_back(row);
    }
}

static bool unpick(double matrix[6][7], double step[6]) {

    for (auto column = 0; column < 6; ++column) {
        auto pivot = column;

        for (auto row = column + 1; row < 6; ++row)
            if (std::fabs(matrix[row][column]) > std::fabs(matrix[pivot][column]))
                pivot = row;

        if (std::fabs(matrix[pivot][column]) < 1e-14)
            return false;

        for (auto k = 0; k < 7; ++k)
            std::swap(matrix[column][k], matrix[pivot][k]);

        for (auto row = 0; row < 6; ++row) {
            if (row == column)
                continue;

            auto factor = matrix[row][column] / matrix[column][column];

            for (auto k = column; k < 7; ++k)
                matrix[row][k] -= factor * matrix[column][k];
        }
    }

    for (auto i = 0; i < 6; ++i)
        step[i] = matrix[i][6] / matrix[i][i];

    return true;
}

bool learnFit(const std::vector<LearnRow> & rows, double values[6]) {

    for (auto k = 0; k < 6; ++k)
        values[k] = 0.0;

    if (rows.empty())
        return false;

    for (auto round = 0; round < static_cast<int>(LEARN_ROUNDS); ++round) {
        double matrix[6][7] = {};

        for (const auto & row : rows) {
            double feature[6];

            for (auto k = 0; k < 5; ++k)
                feature[k] = row.material[k];

            feature[5] = 1.0;

            auto sum = 0.0;

            for (auto k = 0; k < 6; ++k)
                sum += values[k] * feature[k];

            sum = std::max(-30.0, std::min(30.0, sum));

            auto chance = 1.0 / (1.0 + std::exp(-sum));
            auto slope  = chance * (1.0 - chance);
            auto mean   = static_cast<double>(row.score) / (2.0 * row.seen);
            auto wrong  = 2.0 * row.seen * (chance - mean) * slope;
            auto curve  = 2.0 * row.seen * slope * slope;

            for (auto i = 0; i < 6; ++i) {
                matrix[i][6] -= wrong * feature[i];

                for (auto j = 0; j < 6; ++j)
                    matrix[i][j] += curve * feature[i] * feature[j];
            }
        }

        for (auto i = 0; i < 6; ++i)
            matrix[i][i] += 1e-9;

        double step[6];

        if (!unpick(matrix, step))
            return false;

        for (auto i = 0; i < 6; ++i)
            values[i] += step[i];
    }

    return true;
}

int learn(int games, int nodes, unsigned long long seed, int threads) {

    std::vector<LearnRow> rows;

    learnRows(rows, games, nodes, seed, threads);

    double raw[6];

    if (!learnFit(rows, raw)) {
        std::cout << "learn: no position survived to fit" << std::endl;
        return 1;
    }

    long long seen = 0;

    for (const auto & row : rows)
        seen += row.seen;

    //
    //  Only the ratios between the weights can change a move, so the fit is free to settle
    //  on any scale at all. Pinning the pawn at a hundred is what makes the numbers readable
    //

    auto scale = (std::fabs(raw[0]) > 1e-12) ? 100.0 / raw[0] : 0.0;

    int value[6];

    for (auto k = 0; k < 6; ++k)
        value[k] = static_cast<int>(std::floor(raw[k] * scale + 0.5));

    std::cout << "positions " << seen << ", distinct " << rows.size() << std::endl << std::endl;

    for (auto k = 0; k < 5; ++k)
        std::cout << "  " << std::left << std::setw(8) << LEARN_NAMES[k]
                  << std::right << std::setw(6) << value[k] << std::endl;

    std::cout << "  " << std::left << std::setw(8) << "tempo"
              << std::right << std::setw(6) << value[5] << std::endl << std::endl;

    std::cout << "static const Taper PIECE_VALUES[8] = {" << std::endl;
    std::cout << "    Taper(   0,    0),   // EMPTY" << std::endl;
    std::cout << "    Taper(" << std::setw(4) << value[1] << ", " << std::setw(4) << value[1] << "),   // KNIGHT" << std::endl;
    std::cout << "    Taper(" << std::setw(4) << value[0] << ", " << std::setw(4) << value[0] << "),   // PAWN" << std::endl;
    std::cout << "    Taper(   0,    0),   // KING" << std::endl;
    std::cout << "    Taper(   0,    0),   // unused" << std::endl;
    std::cout << "    Taper(" << std::setw(4) << value[2] << ", " << std::setw(4) << value[2] << "),   // BISHOP" << std::endl;
    std::cout << "    Taper(" << std::setw(4) << value[3] << ", " << std::setw(4) << value[3] << "),   // ROOK" << std::endl;
    std::cout << "    Taper(" << std::setw(4) << value[4] << ", " << std::setw(4) << value[4] << ")    // QUEEN" << std::endl;
    std::cout << "};" << std::endl << std::endl;
    std::cout << "    TEMPO           = " << value[5] << std::endl;

    return 0;
}

pasteque_namespace_end

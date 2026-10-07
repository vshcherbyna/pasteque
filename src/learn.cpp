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
#include <chrono>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
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


static const int GRID_SLOT[8] = { -1, 1, 0, 5, -1, 2, 3, 4 };

const unsigned char GRID_KINDS[6] = { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

static const char * const GRID_NAMES[6] = { "PAWN", "KNIGHT", "BISHOP", "ROOK", "QUEEN", "KING" };

static const int GRID_ORDER[6] = { 1, 0, 5, 2, 3, 4 };

static const double GRID_SMOOTH = 1.0e-4;

static const double GRID_MIRROR = 1.0e-4;

static const double GRID_RIDGE = 1.0e-6;

static const double GRID_STRENGTH[GRID_TRIALS] = { 0.25, 1.0, 4.0, 16.0, 64.0 };

void gridTerms(const Board & board, GridSample & sample) {

    auto pieces = board.getAllPieces();
    auto flip   = (board.getSide() == WHITE) ? 1 : -1;

    sample.terms = 0;

    while (pieces) {
        auto square = popFirstOne(pieces);
        auto piece  = board.getPiece(square);
        auto slot   = GRID_SLOT[piece & 7];

        if (slot < 0)
            continue;

        auto white = piece_color(piece) == WHITE;
        auto term  = slot * 64 + (white ? (square ^ 56) : square) + 1;

        sample.term[sample.terms++] = static_cast<short>(white ? flip * term : -flip * term);
    }
}

static bool gridPlayOut(const std::string & opening, const Clock & clock, Search & searcher,
                        std::vector<GridSample> & basket) {

    Board board;

    if (!board.setFen(opening))
        return false;

    std::vector<GridSample> samples;

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

        if (!board.getCheckers() && best.getCapture() == EMPTY && !best.getPromotion() && !starved(board)) {
            GridSample sample;

            gridTerms(board, sample);

            sample.score = board.getSide();
            sample.seen  = 1;

            samples.push_back(sample);
        }

        Rewind undo;

        board.doMove(best, undo);
    }

    if (!ended)
        return false;

    if (samples.size() > static_cast<size_t>(LEARN_KEEP)) {
        std::vector<GridSample> thinned;

        auto stride = static_cast<double>(samples.size()) / LEARN_KEEP;

        for (auto i = 0; i < static_cast<int>(LEARN_KEEP); ++i)
            thinned.push_back(samples[static_cast<size_t>(i * stride)]);

        samples.swap(thinned);
    }

    for (auto & sample : samples) {
        auto mine = (sample.score == WHITE) ? result : -result;

        sample.score = static_cast<unsigned short>(mine + 1);
        basket.push_back(sample);
    }

    return true;
}

void gridSamples(std::vector<GridSample> & samples, int games, int nodes, unsigned long long seed, int threads, GridTally & tally) {

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

    auto ceiling = static_cast<int>(GRID_SAMPLES) / static_cast<int>(LEARN_KEEP);

    if (games > ceiling)
        games = ceiling;

    std::vector<std::string> openings;

    openingPositions(openings, games, OPENINGS_PLIES, seed);

    Judge::init();

    std::vector<std::vector<GridSample>> shares(openings.size());

    std::atomic<size_t> next{0},
                        done{0},
                        reached{0};

    auto started = std::chrono::steady_clock::now();

    auto note = (games >= 20) ? static_cast<size_t>(games / 20) : static_cast<size_t>(games);

    auto labour = [&]() {
        Clock  clock({ "go", "nodes", std::to_string(nodes) }, WHITE);
        Search searcher;

        for (;;) {
            auto index = next++;

            if (index >= openings.size())
                return;

            if (gridPlayOut(openings[index], clock, searcher, shares[index]))
                ++reached;

            auto far = ++done;

            if (note && (far % note) == 0)
                std::cerr << ("grid: " + std::to_string(far) + " of "
                              + std::to_string(openings.size()) + " games\n");
        }
    };

    std::vector<std::thread> hands;

    for (auto i = 0; i < threads; ++i)
        hands.push_back(std::thread(labour));

    for (auto & hand : hands)
        hand.join();

    samples.clear();

    for (size_t game = 0; game < shares.size(); ++game) {
        auto & basket = shares[game];

        for (auto & sample : basket)
            sample.withheld = (game % GRID_HOLDOUT) == 0;

        samples.insert(samples.end(), basket.begin(), basket.end());
    }

    auto span = std::chrono::steady_clock::now() - started;

    tally.played    = static_cast<int>(openings.size());
    tally.concluded = static_cast<int>(reached.load());
    tally.spent     = static_cast<unsigned long long>(
                          std::chrono::duration_cast<std::chrono::milliseconds>(span).count());
}

static bool cholesky(std::vector<double> & matrix, std::vector<double> & rhs, int n) {

    for (auto i = 0; i < n; ++i) {
        for (auto j = 0; j < i; ++j) {
            auto sum = matrix[static_cast<size_t>(i) * n + j];

            for (auto k = 0; k < j; ++k)
                sum -= matrix[static_cast<size_t>(i) * n + k] * matrix[static_cast<size_t>(j) * n + k];

            matrix[static_cast<size_t>(i) * n + j] = sum / matrix[static_cast<size_t>(j) * n + j];
        }

        auto sum = matrix[static_cast<size_t>(i) * n + i];

        for (auto k = 0; k < i; ++k)
            sum -= matrix[static_cast<size_t>(i) * n + k] * matrix[static_cast<size_t>(i) * n + k];

        if (sum <= 0.0)
            return false;

        matrix[static_cast<size_t>(i) * n + i] = std::sqrt(sum);
    }

    for (auto i = 0; i < n; ++i) {
        auto sum = rhs[static_cast<size_t>(i)];

        for (auto k = 0; k < i; ++k)
            sum -= matrix[static_cast<size_t>(i) * n + k] * rhs[static_cast<size_t>(k)];

        rhs[static_cast<size_t>(i)] = sum / matrix[static_cast<size_t>(i) * n + i];
    }

    for (auto i = n - 1; i >= 0; --i) {
        auto sum = rhs[static_cast<size_t>(i)];

        for (auto k = i + 1; k < n; ++k)
            sum -= matrix[static_cast<size_t>(k) * n + i] * rhs[static_cast<size_t>(k)];

        rhs[static_cast<size_t>(i)] = sum / matrix[static_cast<size_t>(i) * n + i];
    }

    return true;
}

//
//  The sum a position stands for, which is the evaluation the fit is trying to reproduce
//

static double gridSum(const GridSample & sample, const double values[GRID_TERMS]) {

    auto sum = values[GRID_TERMS - 1];

    for (auto i = 0; i < sample.terms; ++i) {
        auto term = sample.term[i];

        sum += (term > 0) ? values[term - 1] : -values[-term - 1];
    }

    return std::max(-30.0, std::min(30.0, sum));
}

double gridLoss(const std::vector<GridSample> & samples, const double values[GRID_TERMS]) {

    auto total = 0.0;
    auto seen  = 0.0;

    for (const auto & sample : samples) {
        auto chance = 1.0 / (1.0 + std::exp(-gridSum(sample, values)));
        auto mean   = 0.5 * sample.score / sample.seen;
        auto apart  = chance - mean;

        total += sample.seen * apart * apart;
        seen  += sample.seen;
    }

    return seen ? total / seen : 0.0;
}

struct GridBlock
{
    std::vector<double> matrix,
                        rhs;
};

static void gridGather(const std::vector<GridSample> & samples, const double values[GRID_TERMS],
                       GridBlock & block, size_t from, size_t upto) {

    const auto n = static_cast<int>(GRID_TERMS);

    std::fill(block.matrix.begin(), block.matrix.end(), 0.0);
    std::fill(block.rhs.begin(), block.rhs.end(), 0.0);

    for (auto i = from; i < upto; ++i) {
        const auto & sample = samples[i];

        int    where[33];
        double sign[33];

        auto count = 0;

        for (auto k = 0; k < sample.terms; ++k) {
            auto term = sample.term[k];

            where[count] = (term > 0) ? term - 1 : -term - 1;
            sign[count]  = (term > 0) ? 1.0 : -1.0;
            ++count;
        }

        where[count] = n - 1;
        sign[count]  = 1.0;
        ++count;

        auto seen   = static_cast<double>(sample.seen);
        auto chance = 1.0 / (1.0 + std::exp(-gridSum(sample, values)));
        auto slope  = chance * (1.0 - chance);
        auto mean   = 0.5 * sample.score / seen;
        auto wrong  = 2.0 * seen * (chance - mean) * slope;
        auto curve  = 2.0 * seen * slope * slope;

        for (auto a = 0; a < count; ++a) {
            block.rhs[static_cast<size_t>(where[a])] -= wrong * sign[a];

            for (auto b = 0; b < count; ++b)
                block.matrix[static_cast<size_t>(where[a]) * n + where[b]] += curve * sign[a] * sign[b];
        }
    }
}

bool gridFit(const std::vector<GridSample> & samples, double values[GRID_TERMS], double strength,
             int threads, double & settled) {

    const auto n = static_cast<int>(GRID_TERMS);

    for (auto k = 0; k < n; ++k)
        values[k] = 0.0;

    if (samples.empty())
        return false;

    auto smooth = GRID_SMOOTH * strength * static_cast<double>(samples.size());
    auto mirror = GRID_MIRROR * strength * static_cast<double>(samples.size());
    auto ridge  = GRID_RIDGE * static_cast<double>(samples.size());

    std::vector<double> matrix(static_cast<size_t>(n) * n),
                        rhs(static_cast<size_t>(n));

    auto blocks = static_cast<size_t>(GRID_BLOCKS);

    if (blocks > samples.size())
        blocks = samples.size();

    std::vector<GridBlock> parts(blocks);

    for (auto & part : parts) {
        part.matrix.resize(static_cast<size_t>(n) * n);
        part.rhs.resize(static_cast<size_t>(n));
    }

    if (threads < 1) {
        threads = static_cast<int>(std::thread::hardware_concurrency());

        if (threads < 1)
            threads = 1;
    }

    auto hands = static_cast<size_t>(threads);

    if (hands > blocks)
        hands = blocks;

    settled = 0.0;

    for (auto round = 0; round < static_cast<int>(GRID_ROUNDS); ++round) {
        std::atomic<size_t> next{0};

        auto labour = [&]() {
            for (;;) {
                auto which = next++;

                if (which >= blocks)
                    return;

                gridGather(samples, values, parts[which],
                           samples.size() * which / blocks,
                           samples.size() * (which + 1) / blocks);
            }
        };

        std::vector<std::thread> crew;

        for (size_t k = 0; k < hands; ++k)
            crew.push_back(std::thread(labour));

        for (auto & hand : crew)
            hand.join();

        std::fill(matrix.begin(), matrix.end(), 0.0);
        std::fill(rhs.begin(), rhs.end(), 0.0);

        for (const auto & part : parts) {
            for (size_t k = 0; k < matrix.size(); ++k)
                matrix[k] += part.matrix[k];

            for (size_t k = 0; k < rhs.size(); ++k)
                rhs[k] += part.rhs[k];
        }

        for (auto slot = 0; slot < 6; ++slot)
            for (auto square = 0; square < 64; ++square) {
                auto here = slot * 64 + square;

                for (auto step = 0; step < 2; ++step) {
                    if (step == 0 && (square & 7) == 7)
                        continue;

                    if (step == 1 && (square >> 3) == 7)
                        continue;

                    auto next = here + (step ? 8 : 1);

                    matrix[static_cast<size_t>(here) * n + here] += 2.0 * smooth;
                    matrix[static_cast<size_t>(next) * n + next] += 2.0 * smooth;
                    matrix[static_cast<size_t>(here) * n + next] -= 2.0 * smooth;
                    matrix[static_cast<size_t>(next) * n + here] -= 2.0 * smooth;

                    rhs[static_cast<size_t>(here)] -= 2.0 * smooth * (values[here] - values[next]);
                    rhs[static_cast<size_t>(next)] -= 2.0 * smooth * (values[next] - values[here]);
                }
            }

        for (auto slot = 0; slot < 6; ++slot)
            for (auto square = 0; square < 64; ++square) {
                if ((square & 7) >= 4)
                    continue;

                auto here = slot * 64 + square;
                auto over = slot * 64 + (square & ~7) + (7 - (square & 7));

                matrix[static_cast<size_t>(here) * n + here] += 2.0 * mirror;
                matrix[static_cast<size_t>(over) * n + over] += 2.0 * mirror;
                matrix[static_cast<size_t>(here) * n + over] -= 2.0 * mirror;
                matrix[static_cast<size_t>(over) * n + here] -= 2.0 * mirror;

                rhs[static_cast<size_t>(here)] -= 2.0 * mirror * (values[here] - values[over]);
                rhs[static_cast<size_t>(over)] -= 2.0 * mirror * (values[over] - values[here]);
            }

        for (auto k = 0; k < n; ++k) {
            matrix[static_cast<size_t>(k) * n + k] += 2.0 * ridge;
            rhs[static_cast<size_t>(k)] -= 2.0 * ridge * values[k];
        }

        if (!cholesky(matrix, rhs, n))
            return false;

        settled = 0.0;

        for (auto k = 0; k < n; ++k) {
            values[k] += rhs[static_cast<size_t>(k)];
            settled = std::max(settled, std::fabs(rhs[static_cast<size_t>(k)]));
        }
    }

    return true;
}

class GridTee : public std::streambuf
{
public:
    GridTee(std::streambuf * console, std::streambuf * file) : m_console(console), m_file(file) {
    }

protected:
    int overflow(int c) override {
        if (c == EOF)
            return 0;

        auto spoken = m_console->sputc(static_cast<char>(c));
        auto kept   = m_file->sputc(static_cast<char>(c));

        return (spoken == EOF || kept == EOF) ? EOF : c;
    }

    int sync() override {
        return (m_console->pubsync() == 0 && m_file->pubsync() == 0) ? 0 : -1;
    }

private:
    std::streambuf * m_console,
                   * m_file;
};

std::string gridSheet(int games, int nodes, unsigned long long seed) {

    std::ostringstream name;

    name << "grid-" << ENGINE_VERSION << "-" << games << "-" << nodes << "-" << seed << ".txt";

    return name.str();
}

static double gridMirror(const int table[64], double & spread, int & worst) {

    auto square = 0.0;
    auto apart  = 0.0;

    worst = 0;

    for (auto row = 0; row < 8; ++row)
        for (auto file = 0; file < 8; ++file) {
            auto here = table[row * 8 + file];
            auto over = table[row * 8 + (7 - file)];

            square += static_cast<double>(here) * here;
            apart  += static_cast<double>(here - over) * (here - over);

            if (std::abs(here) > std::abs(worst))
                worst = here;
        }

    spread = std::sqrt(square / 64.0);

    return spread ? std::sqrt(apart / 64.0) / spread : 0.0;
}

int gridVictim(const int value[5], int & narrowest, int & spread) {

    int sorted[5];

    for (auto k = 0; k < 5; ++k)
        sorted[k] = value[k];

    std::sort(sorted, sorted + 5);

    spread    = sorted[4] - sorted[0];
    narrowest = 0;

    for (auto k = 0; k + 1 < 5; ++k) {
        auto gap = sorted[k + 1] - sorted[k];

        if (gap > 0 && (narrowest == 0 || gap < narrowest))
            narrowest = gap;
    }

    return narrowest ? spread / narrowest + 1 : 0;
}

static void gridOrdering(std::ostream & sheet, const int value[6]) {

    static const char * const GRID_RANKED[5] = { "pawn", "knight", "bishop", "rook", "queen" };

    int narrowest,
        spread;

    auto least = gridVictim(value, narrowest, spread);

    if (!least) {
        sheet << "ordering  every victim is worth the same, so nothing constrains ORDER_VICTIM"
              << std::endl << std::endl;
        return;
    }

    //  name the pair that binds, found the same way the rule measures it

    int rank[5] = { 0, 1, 2, 3, 4 };

    std::sort(rank, rank + 5, [&](int a, int b) { return value[a] < value[b]; });

    auto under = 0,
         over  = 1;

    for (auto k = 0; k + 1 < 5; ++k)
        if (value[rank[k + 1]] - value[rank[k]] == narrowest) {
            under = k;
            over  = k + 1;
            break;
        }

    sheet << "ordering  narrowest victim gap " << narrowest << " ("
          << GRID_RANKED[rank[under]] << " " << value[rank[under]] << " to "
          << GRID_RANKED[rank[over]] << " " << value[rank[over]] << "), attacker spread "
          << spread << std::endl
          << "          needs ORDER_VICTIM of at least " << least << ", search carries "
          << static_cast<int>(ORDER_VICTIM) << " -- "
          << ((static_cast<int>(ORDER_VICTIM) >= least) ? "clears" : "** VIOLATED **")
          << std::endl << std::endl;
}

static void gridPrint(std::ostream & sheet, const std::string & name, const int table[64]) {

    sheet << "static const int " << name << "[64] = {" << std::endl;

    for (auto row = 0; row < 8; ++row) {
        sheet << "   ";

        for (auto file = 0; file < 8; ++file) {
            sheet << std::setw(5) << table[row * 8 + file];

            if (row * 8 + file != 63)
                sheet << ",";
        }

        sheet << std::endl;
    }

    sheet << "};" << std::endl << std::endl;
}

int grid(int games, int nodes, unsigned long long seed, int threads, double strength) {

    std::vector<GridSample> samples;
    GridTally               tally;

    gridSamples(samples, games, nodes, seed, threads, tally);

    //
    //  Every fifth opening is withheld with all of its retained positions, so no game can
    //  contribute to both the fit and its validation
    //

    std::vector<GridSample> taught,
                            withheld;

    for (const auto & sample : samples)
        (sample.withheld ? withheld : taught).push_back(sample);

    std::vector<double> values(static_cast<size_t>(GRID_TERMS)),
                        trial(static_cast<size_t>(GRID_TERMS));

    auto name = gridSheet(games, nodes, seed);

    std::ofstream file(name);

    if (!file)
        std::cout << "grid: cannot write " << name << ", reporting to the console only"
                  << std::endl;

    GridTee        tee(std::cout.rdbuf(), file.rdbuf());
    std::ostream   teed(&tee);
    std::ostream & sheet = file ? teed : std::cout;

    sheet << ENGINE_NAME << " " << ENGINE_VERSION << " " << ENGINE_ARCH << " grid" << std::endl
          << "games " << games << ", nodes " << nodes << ", seed " << seed
          << std::endl << std::endl;

    sheet << "games " << tally.concluded << " of " << tally.played << " concluded"
          << " (" << (tally.played - tally.concluded) << " discarded), "
          << tally.spent / 1000 << " s" << std::endl
          << "positions " << samples.size() << ", withheld " << withheld.size()
          << ", " << (samples.size() / GRID_TERMS) << " per parameter" << std::endl << std::endl;

    if (taught.empty() || withheld.empty()) {
        sheet << "grid: need completed games with retained positions in both training and holdout sets"
                  << std::endl;
        return 1;
    }

    //
    //  The loss is nearly flat in the strength because it is dominated by material, so it
    //  reports rather than decides when a strength has been named. Asked for one, it sweeps
    //

    if (strength <= 0.0) {
        auto best  = 0.0;
        auto found = -1;

        for (auto which = 0; which < static_cast<int>(GRID_TRIALS); ++which) {
            auto settled = 0.0;

            if (!gridFit(taught, trial.data(), GRID_STRENGTH[which], threads, settled))
                continue;

            auto loss = gridLoss(withheld, trial.data());

            sheet << "  strength " << std::setw(6) << GRID_STRENGTH[which]
                      << "   held-out loss " << std::setprecision(8) << loss
                      << "   settled " << settled << std::endl;

            if (found < 0 || loss < best) {
                best  = loss;
                found = which;
            }
        }

        if (found < 0) {
            sheet << "grid: nothing to fit" << std::endl;
            return 1;
        }

        strength = GRID_STRENGTH[found];
        sheet << std::endl;
    }
    else {
        auto settled = 0.0;

        if (!gridFit(taught, trial.data(), strength, threads, settled)) {
            sheet << "grid: nothing to fit" << std::endl;
            return 1;
        }

        sheet << "  strength " << std::setw(6) << strength << "   held-out loss "
                  << std::setprecision(8) << gridLoss(withheld, trial.data())
                  << "   settled " << settled << std::endl << std::endl;
    }

    sheet << "strength " << strength << ", refitted on every position"
              << std::endl << std::endl;

    auto settled = 0.0;
    auto fitting = std::chrono::steady_clock::now();

    if (!gridFit(samples, values.data(), strength, threads, settled)) {
        sheet << "grid: nothing to fit" << std::endl;
        return 1;
    }

    auto spent = std::chrono::steady_clock::now() - fitting;

    sheet << "settled " << std::setprecision(4) << settled << " on the last of "
          << static_cast<int>(GRID_ROUNDS) << " rounds, fitted in "
          << std::chrono::duration_cast<std::chrono::milliseconds>(spent).count()
          << " ms" << std::endl << std::endl;

    //
    //  The mean of a table is the value of the piece and what is left over is its shape. The
    //  overall scale cannot change a move choice, so it is pinned by the pawn as before
    //

    double centre[6];

    for (auto slot = 0; slot < 6; ++slot) {
        auto sum = 0.0;

        for (auto square = 0; square < 64; ++square)
            sum += values[static_cast<size_t>(slot) * 64 + square];

        centre[slot] = sum / 64.0;
    }

    auto scale = (std::fabs(centre[0]) > 1.0e-12) ? 100.0 / centre[0] : 0.0;

    for (auto slot = 0; slot < 6; ++slot)
        sheet << "  " << std::left << std::setw(8) << GRID_NAMES[slot] << std::right
                  << std::setw(6) << static_cast<int>(std::floor(centre[slot] * scale + 0.5))
                  << std::endl;

    sheet << "  " << std::left << std::setw(8) << "tempo" << std::right << std::setw(6)
              << static_cast<int>(std::floor(values[GRID_TERMS - 1] * scale + 0.5))
              << std::endl << std::endl;

    int shaped[6][64];

    for (auto slot = 0; slot < 6; ++slot)
        for (auto square = 0; square < 64; ++square) {
            auto shape = values[static_cast<size_t>(slot) * 64 + square] - centre[slot];
            auto never = (slot == 0) && ((square >> 3) == 0 || (square >> 3) == 7);

            shaped[slot][square] = never ? 0 : static_cast<int>(std::floor(shape * scale + 0.5));
        }

    sheet << "table      mirror    size   worst" << std::endl;

    for (auto slot = 0; slot < 6; ++slot) {
        double spread;
        int    worst;

        auto mirror = gridMirror(shaped[slot], spread, worst);

        sheet << "  " << std::left << std::setw(8) << GRID_NAMES[slot] << std::right
              << std::setw(7) << std::fixed << std::setprecision(2) << mirror
              << std::setw(8) << std::setprecision(1) << spread
              << std::setw(8) << worst << std::endl;
    }

    sheet.unsetf(std::ios::floatfield);
    sheet << std::endl;

    int order[6];

    for (auto slot = 0; slot < 6; ++slot)
        order[slot] = static_cast<int>(std::floor(centre[slot] * scale + 0.5));

    gridOrdering(sheet, order);

    for (auto which = 0; which < 6; ++which) {
        auto slot = GRID_ORDER[which];

        gridPrint(sheet, std::string(GRID_NAMES[slot]) + "_OPENING", shaped[slot]);
        gridPrint(sheet, std::string(GRID_NAMES[slot]) + "_CLOSING", shaped[slot]);
    }

    if (file)
        std::cout << "written to " << name << std::endl;

    return 0;
}

pasteque_namespace_end

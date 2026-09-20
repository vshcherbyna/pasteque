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

#include <chrono>
#include <iomanip>
#include <iostream>

#include "bench.h"
#include "board.h"
#include "search.h"

pasteque_namespace_begin

//
//  Random legal positions, reached by playing random legal moves out of the standard
//  opening position. They were produced by patches-pasteque/genbench.py spending the
//  operating system's cryptographic random source on the move choices, so the list owes
//  nothing to anyone's taste in chess. Re-running the generator replaces them wholesale,
//  which moves the node count and is therefore a deliberate act, not a tidy-up
//

static const char * const POSITIONS[] = {
    "1n1qk2r/r1p3bp/p2ppn2/1p3pp1/2bP2P1/PPN1PP1N/2P3BP/1RBQK2R w Kk - 4 16",
    "1nbq2r1/1p2Bk2/2pp4/1p5p/r2P1pnP/4P3/P1P2PP1/RN1K1B1R w - - 1 21",
    "r1bB1b2/6kr/p1npp3/1p1P4/PPP1PpnP/N6P/3K1P2/2R1QB1R w - - 4 28",
    "5r2/5kpr/1p3p1n/2Q1p2P/pb3P2/P1P2b2/1P1BPNBP/RN2KR2 b Q - 0 26",
    "r2qkB1r/p1p4p/1p1Pp1n1/1P1N1p2/3Pb3/N7/P1Q1PPP1/3RKn1R w K - 0 24",
    "1r2qrk1/2pb2pp/P2bp3/8/2B1Pp2/NP3NP1/P2P1P1R/RKB5 w - - 1 24",
    "r1bqk1n1/1p1ppp1r/p1n2bp1/2pB2Pp/8/1PP1PQ2/P2P1P1P/RNB1K1NR w KQq - 6 9",
    "2q1k1nr/2rp1p2/n1p1p3/Q1P1P2R/ppBP1Pp1/P7/1P5N/RNB1K3 b Qk - 4 24",
    "r1bqk3/1p1n1ppr/p4R2/3pp2p/Pb2P1P1/8/2PP1P1P/1NBQKBNR w Kq - 1 13",
    "rnbqkbnr/3pp2p/5p2/ppp3p1/5P1P/PP2P1PB/2PP1K2/RNBQ2NR b kq - 4 8",
    "rq3k1r/p3ppbp/n3b2n/2pPp3/2P3P1/1RP5/P2BBKPP/N5NR b - - 11 25",
    "r1b1kb1r/pppn1p2/4p1p1/n2p2q1/4P1Pp/P2P1Q1P/RPPB1P2/1N2KBNR w q - 2 13",
    "rn2kbn1/pppb1ppr/B2pp3/P6p/2P1P3/5NPP/1P1PKP2/RNBQ3R b q - 0 10",
    "3q1b2/nrpbk1p1/4p2r/pp1P2Np/P2PPp1P/1P3PP1/3B2Bn/RN1QK3 w - - 1 19",
    "1k3b1r/p1p1ppp1/2n2r2/2NQ1Pq1/b1p3B1/P4KPP/8/R1B3NR b - - 6 27",
    "1r1k3r/pb5p/1pp5/1P5P/P1pPQ3/b3PPq1/1B2B3/1N1K2NR w - - 0 31"
};

int benchCount() {
    return static_cast<int>(sizeof(POSITIONS) / sizeof(POSITIONS[0]));
}

const char * benchPosition(int index) {
    return POSITIONS[index];
}

//
//  The last line is the one OpenBench reads, and it is the only line in the output that
//  carries the words it looks for. Everything above it is for a human comparing two
//  runs and wondering which position moved
//

int bench(int depth) {

    if (depth < 1)
        depth = BENCH_DEPTH;

    std::cout << "position   depth          count  move" << std::endl;

    unsigned long long counted = 0;
    auto started = std::chrono::steady_clock::now();

    for (auto i = 0; i < benchCount(); ++i) {

        Board  board;
        Search searcher;

        if (!board.setFen(POSITIONS[i])) {
            std::cout << "bench: position " << (i + 1) << " will not parse" << std::endl;
            return 1;
        }

        auto best = searcher.bestMove(board, depth);

        counted += searcher.getNodes();

        std::cout << std::setw(8) << (i + 1)
                  << std::setw(8) << depth
                  << std::setw(15) << searcher.getNodes()
                  << "  " << best.toString() << std::endl;
    }

    auto spent = std::chrono::duration_cast<std::chrono::milliseconds>
                     (std::chrono::steady_clock::now() - started).count();

    std::cout << std::endl << "elapsed " << spent << " ms" << std::endl << std::endl;

    std::cout << counted << " nodes " << (spent ? counted * 1000 / static_cast<unsigned long long>(spent) : 0)
              << " nps" << std::endl;

    return 0;
}

pasteque_namespace_end

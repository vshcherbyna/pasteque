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

#include <cstdlib>

#include "clock.h"
#include "bitboard.h"

pasteque_namespace_begin

static unsigned int number(const std::string & text) {

    auto value = std::atoi(text.c_str());

    return (value > 0) ? static_cast<unsigned int>(value) : 0;
}

static unsigned int trim(unsigned long long want, unsigned int pool) {

    if (pool <= MOVE_OVERHEAD)
        return 1;

    unsigned long long ceiling = pool - MOVE_OVERHEAD;

    if (want > ceiling)
        return static_cast<unsigned int>(ceiling);

    return want ? static_cast<unsigned int>(want) : 1;
}

Clock::Clock() : m_soft{0}, m_hard{0}, m_depth{DEPTH_CEILING}, m_endless{true} {
}

Clock::Clock(unsigned int soft, unsigned int hard, int depth)
    : m_soft{soft}, m_hard{hard}, m_depth{depth}, m_endless{false} {
}

Clock::Clock(const std::vector<std::string> & tokens, unsigned char side) : Clock() {

    const char * ours  = (side == WHITE) ? "wtime" : "btime";
    const char * bonus = (side == WHITE) ? "winc"  : "binc";

    unsigned int remaining = 0,
                 gain      = 0,
                 fixed     = 0,
                 togo      = 0;

    auto bounded   = false,
         haveFixed = false,
         haveClock = false;

    for (size_t i = 1; i + 1 < tokens.size(); ++i) {

        const auto & name  = tokens[i];
        const auto & value = tokens[i + 1];

        if (name == ours) {
            remaining = number(value);
            haveClock = true;
            bounded   = true;
        }
        else if (name == bonus)
            gain = number(value);
        else if (name == "movestogo")
            togo = number(value);
        else if (name == "movetime") {
            fixed     = number(value);
            haveFixed = true;
            bounded   = true;
        }
        else if (name == "depth") {
            m_depth = std::atoi(value.c_str());
            bounded = true;
        }
        else
            continue;

        ++i;
    }

    if (m_depth < 1)
        m_depth = 1;

    if (m_depth > DEPTH_CEILING)
        m_depth = DEPTH_CEILING;

    if (!bounded) {
        m_depth = DEFAULT_DEPTH;
        return;
    }

    if (haveFixed) {
        m_soft    = trim(fixed, fixed);
        m_hard    = m_soft;
        m_endless = false;

        return;
    }

    if (!haveClock)
        return;

    auto share = togo ? (remaining / togo) : (remaining / SUDDEN_DEATH_MOVES);

    m_soft    = trim(static_cast<unsigned long long>(share) + (gain * 3) / 4, remaining);
    m_hard    = trim(static_cast<unsigned long long>(m_soft) * HARD_MULTIPLIER, remaining);
    m_endless = false;
}

pasteque_namespace_end

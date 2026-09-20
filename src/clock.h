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

#ifndef CLOCK_H
#define CLOCK_H

#include <chrono>
#include <string>
#include <vector>

#include "pasteque.h"

pasteque_namespace_begin

typedef std::chrono::steady_clock::time_point Instant;

enum
{
    DEFAULT_DEPTH       = 6,
    DEPTH_CEILING       = 64,
    MOVE_OVERHEAD       = 30,
    SUDDEN_DEATH_MOVES  = 30,
    HARD_MULTIPLIER     = 3
};

class Clock
{
public:
    Clock();
    Clock(unsigned int soft, unsigned int hard, int depth);
    Clock(const std::vector<std::string> & tokens, unsigned char side);

public:
    unsigned int    getSoft() const { return m_soft; }
    unsigned int    getHard() const { return m_hard; }
    int             getDepth() const { return m_depth; }
    bool            isEndless() const { return m_endless; }

private:
    unsigned int    m_soft,
                    m_hard;

    int             m_depth;
    bool            m_endless;
};

pasteque_namespace_end
#endif // CLOCK_H

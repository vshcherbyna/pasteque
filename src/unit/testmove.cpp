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

#include <gtest/gtest.h>

#include "../move.h"

pasteque_namespace_begin
namespace unit
{

TEST(Move, Positive)
{
    Move m{};
    EXPECT_EQ(m, 0);
}

TEST(Move_from, Positive)
{
    Move m{};
    EXPECT_EQ(m, 0);

    m.from(1);
    EXPECT_NE(m, 0);
}

TEST(Move_to, Positive)
{
    Move m{};
    EXPECT_EQ(m, 0);

    m.to(1);
    EXPECT_NE(m, 0);
}

TEST(Move_piece, Positive)
{
    Move m{};
    EXPECT_EQ(m, 0);

    m.piece(1);
    EXPECT_NE(m, 0);
}

TEST(Move_capture, Positive)
{
    Move m{};
    EXPECT_EQ(m, 0);

    m.capture(1);
    EXPECT_NE(m, 0);
}

}
pasteque_namespace_end

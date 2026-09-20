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

#include "perft.h"
#include "moves.h"

pasteque_namespace_begin

unsigned long long perft(Board & board, int depth) {

    if (depth <= 0)
        return 1;

    Moves moves;
    moves.generateLegal(board);

    if (depth == 1)
        return static_cast<unsigned long long>(moves.size());

    unsigned long long nodes = 0;

    for (auto i = 0; i < moves.size(); ++i) {
        Rewind undo;

        board.doMove(moves[i], undo);
        nodes += perft(board, depth - 1);
        board.unmakeMove(moves[i], undo);
    }

    return nodes;
}

pasteque_namespace_end

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

#include "uci.h"
#include <iostream>
#include <string>

pasteque_namespace_begin

int Uci::handleCmdLine(int argc, char *argv[])
{
    std::string line;
    std::cout.setf(std::ios::unitbuf);

    std::cout << "pasteque 0.0 by Volodymyr Shcherbyna" << std::endl;

    while (std::getline(std::cin, line))
    {
        if (line == "uci")
            onUci();
        else if (line == "quit")
            onQuit();
        if (line == "isready")
            onIsReady();
        else if (line == "ucinewgame")
            onNewGame();
        else if (line == "position")
            onPosition();
    }

    return 0;
}

void Uci::onUci()
{
    std::cout << "id name igel 1.0" << std::endl;
    std::cout << "id author Volodymyr Shcherbyna" << std::endl;
    std::cout << "uciok" << std::endl;
}

void Uci::onQuit()
{

}

void Uci::onIsReady()
{
    std::cout << "readyok" << std::endl;
}

void Uci::onNewGame()
{
}

void Uci::onPosition()
{

}

pasteque_namespace_end

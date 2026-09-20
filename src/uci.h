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

#ifndef UCI_H
#define UCI_H

#include <string>
#include <vector>

#include "pasteque.h"
#include "board.h"
#include "clock.h"

pasteque_namespace_begin

class Uci
{
public:
    Uci();

public:
    int  handleCmdLine(int argc, char *argv[]);

public:
    void handleCommand(const std::string & line);

    const Board & getBoard() const { return m_board; }
    bool  departing() const { return m_departing; }

private:
    void onUci();
    void onQuit();
    void onIsReady();
    void onNewGame();
    void onPosition(const std::vector<std::string> & tokens);
    void onGo(const std::vector<std::string> & tokens);
    void onBench(const std::vector<std::string> & tokens);

private:
    bool playMove(const std::string & notation);

private:
    Board m_board;
    bool  m_departing;
};

pasteque_namespace_end
#endif // UCI_H

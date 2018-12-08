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

#ifndef UCI_H
#define UCI_H

#include "pasteque.h"

pasteque_namespace_begin

class Uci
{
public:
    int handleCmdLine(int argc, char *argv[]);

private:
    void onUci();
    void onQuit();
    void onIsReady();
    void onNewGame();
    void onPosition();
};

pasteque_namespace_end
#endif // UCI_H

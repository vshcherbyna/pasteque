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

#include <cstdlib>
#include <iostream>

#include "uci.h"
#include "moves.h"
#include "search.h"
#include "judge.h"

pasteque_namespace_begin

static void tokenize(const std::string & line, std::vector<std::string> & tokens) {
    std::string token;

    tokens.clear();

    for (auto character : line) {
        if (character == ' ' || character == '\t' || character == '\r') {
            if (!token.empty())
                tokens.push_back(token);

            token.clear();
        }
        else
            token += character;
    }

    if (!token.empty())
        tokens.push_back(token);
}

static void reportScore(int score)
{
    if (score > MATE_SCORE - PLY_LIMIT)
        std::cout << "score mate " << (MATE_SCORE - score + 1) / 2;
    else if (score < -MATE_SCORE + PLY_LIMIT)
        std::cout << "score mate " << -((MATE_SCORE + score + 1) / 2);
    else
        std::cout << "score cp " << score;
}

static void report(int depth, int score, unsigned long long nodes, unsigned long long msec, Move best) {

    std::cout << "info depth " << depth << " ";

    reportScore(score);

    std::cout << " nodes " << nodes << " time " << msec;

    if (msec)
        std::cout << " nps " << (nodes * 1000 / msec);

    std::cout << " pv " << best.toString() << std::endl;
}

Uci::Uci() : m_departing{false} {
    Judge::init();
}

int Uci::handleCmdLine(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    std::string line;

    std::cout.setf(std::ios::unitbuf);

    onUci();

    while (!m_departing && std::getline(std::cin, line))
        handleCommand(line);

    return 0;
}

void Uci::handleCommand(const std::string & line) {
    std::vector<std::string> tokens;

    tokenize(line, tokens);

    if (tokens.empty())
        return;

    const auto & command = tokens[0];

    if (command == "uci")
        onUci();
    else if (command == "isready")
        onIsReady();
    else if (command == "ucinewgame")
        onNewGame();
    else if (command == "position")
        onPosition(tokens);
    else if (command == "go")
        onGo(tokens);
    else if (command == "quit")
        onQuit();
}

void Uci::onUci() {
    std::cout << "id name pasteque 0.0.0" << std::endl;
    std::cout << "id author Volodymyr Shcherbyna" << std::endl;
    std::cout << "uciok" << std::endl;
}

void Uci::onQuit() {
    m_departing = true;
}

void Uci::onIsReady() {
    std::cout << "readyok" << std::endl;
}

void Uci::onNewGame() {
    m_board.setInitial();
}

bool Uci::playMove(const std::string & notation) {

    Moves moves;
    moves.generateLegal(m_board);

    for (auto i = 0; i < moves.size(); ++i) {
        if (moves[i].toString() != notation)
            continue;

        Rewind rewind;

        m_board.doMove(moves[i], rewind);

        return true;
    }

    return false;
}

void Uci::onPosition(const std::vector<std::string> & tokens) {
    size_t i = 1;

    if (i < tokens.size() && tokens[i] == "startpos") {
        m_board.setInitial();
        ++i;
    }
    else if (i < tokens.size() && tokens[i] == "fen") {
        std::string fen;

        for (++i; i < tokens.size() && tokens[i] != "moves"; ++i) {
            if (!fen.empty())
                fen += ' ';

            fen += tokens[i];
        }

        if (!m_board.setFen(fen)) {
            std::cout << "info string cannot read that fen" << std::endl;
            return;
        }
    }
    else
        return;

    if (i >= tokens.size() || tokens[i] != "moves")
        return;

    for (++i; i < tokens.size(); ++i) {
        if (!playMove(tokens[i])) {
            std::cout << "info string " << tokens[i] << " is not legal here" << std::endl;
            return;
        }
    }
}

void Uci::onGo(const std::vector<std::string> & tokens) {

    Clock clock(tokens, m_board.getSide());

    Search searcher;

    searcher.setWatcher(report);

    auto best = searcher.bestMove(m_board, clock);

    if (static_cast<int>(best) == 0)
        std::cout << "bestmove 0000" << std::endl;
    else
        std::cout << "bestmove " << best.toString() << std::endl;
}

pasteque_namespace_end

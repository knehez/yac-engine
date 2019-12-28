#include "engine.h"
#include <thread>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

void Engine::setFENCode(const char *fen)
{
    board.setFENCode(fen);
}

void Engine::handleUCI()
{
    std::string strUCI;

    while (true)
    {
        getline(std::cin, strUCI);

        if (strUCI == "uci")
        {
            std::cout << "id name Yacengine\n";
            std::cout << "id author Karoly Nehez\n";
            //options go here
            std::cout << "uciok\n";
            continue;
        }
        if (strUCI == "isready")
        {
            std::cout << "readyok\n";
            continue;
        }
        if (strUCI == "ucinewgame")
        {
            continue;
        }
        if (strUCI.rfind("go", 0) == 0)
        {
            auto bestMove = go();
            std::cout << "\nbestmove " << board.to_string(bestMove) << "\n";
        }
        if (strUCI.rfind("position", 0) == 0)
        {
            strUCI = strUCI.substr(9);
            if (strUCI.find("startpos ") != std::string::npos)
            {
                board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                board.initialState();
            }
            else if (strUCI.find("fen") != std::string::npos)
            {
                strUCI = strUCI.substr(4);
                board.setFENCode(strUCI.c_str());
                board.initialState();
            }
            if (strUCI.find("moves") != std::string::npos)
            {
                strUCI = strUCI.substr(strUCI.find("moves") + 6);

                std::stringstream ss(strUCI);
                std::vector<std::string> splittedMoves;

                while (ss.good())
                {
                    std::string substr;
                    getline(ss, substr, ' ');
                    splittedMoves.push_back(substr);
                }

                for (auto move : splittedMoves)
                {
                    Move m;
                    parseMove(move, &m);
                    board.validateMove(&m);
                    board.move(m);
                }
            }
            continue;
        }
    }
}

void Engine::start()
{
    std::thread uci(&Engine::handleUCI, this);
    uci.join();
}

Move Engine::go()
{
    Moves moves;
    Color myColor = board.getColor();
    std::cout << "info color=" << (myColor == WHITE ? "WHITE" : "BLACK");
    moves = board.generateMoves(moves);
    std::vector<int> v;
    for (int i = 0; i < moves.length; i++)
    {
        board.move(moves.move[i]);
        v.push_back(monteCarloSearch(board.getFENCode().c_str()));
        board.undoMove(moves.move[i]);
    }

    int elementIndex = 0;
    if (myColor == WHITE)
    {
        elementIndex = (int)(std::min_element(v.begin(), v.end()) - v.begin());
    }
    else
    {
        elementIndex = (int)(std::max_element(v.begin(), v.end()) - v.begin());
    }
    board.move(moves.move[elementIndex]);
    return moves.move[elementIndex];
}

int Engine::monteCarloSearch(const char *fen)
{
    uint64_t count = 0;
    int sumValue = 0;
    int value = 0;
    bool isMateFound = false;
    for (int i = 0; i < 1000; i++)
    {
        count += monteCarloSimulation(12, isMateFound, value);
        if (board.getFENCode() != fen)
        {
            std::cout << "info ******************** DIFFERENT FEN! **************************";
        }
        if (isMateFound)
        {
            sumValue += value;
            value = 0;
            isMateFound = false;
            board.setFENCode(fen);
        }
    }
    return sumValue;
}

uint64_t Engine::monteCarloSimulation(int depth, bool &isMateFound, int &value)
{
    uint64_t count = 0;
    Moves moves;

    moves = board.generateMoves(moves);

    // no moves then mate (or stalemate)
    if (moves.length == 0)
    {
        isMateFound = true;
        std::cout << "\n"
                  << board.getFENCode() << "\n";
        std::cout << (board.getColor() == WHITE ? "WHITE" : "BLACK") << "\n";
        value = board.getColor() == WHITE ? 1 : -1;
        return 1;
    }
    int randomIndex = rand() % moves.length;

    if (depth == 1)
    {
        return 1;
    }
    move(moves.move[randomIndex]);

    count += monteCarloSimulation(depth - 1, isMateFound, value);

    board.undoMove(moves.move[randomIndex]);

    if (isMateFound)
    {
        std::cout << board.to_string(moves.move[randomIndex]) << " ";
    }

    return count;
}

uint64_t Engine::perft(int depth)
{
    uint64_t count = 0;
    Moves moves;

    moves = board.generateMoves(moves);
    if (depth == 1)
    {
        /*for (int i = 0; i < moves.length; i++)
        {
            for (auto m : board.matchMoves)
            {
                board.fenFile << board.to_string(m) << '|';
            }
            board.fenFile << board.to_string(moves.move[i]) << "\n";
        }*/

        return moves.length;
    }
    for (int i = 0; i < moves.length; i++)
    {
        board.move(moves.move[i]);
        count += perft(depth - 1);
        board.undoMove(moves.move[i]);
    }
    return count;
}

void Engine::parseMove(std::string move, Move *m)
{
    m->start = (Position)(((move[1] - '1') << 3) + (move[0] - 'a'));
    m->end = (Position)(((move[3] - '1') << 3) + (move[2] - 'a'));
    if (move.length() == 5)
    {
        if (move[4] == 'r')
        {
            // TODO: handle promotions...
        }
    }
}

#include "engine.h"
#include <thread>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include "mcts.h"

void Engine::setFENCode(const char *fen)
{
    board.setInitialState();
    board.setFENCode(fen);
}

void Engine::move(Move m)
{
    board.validateMove(&m); // TODO: invalid move?
    board.move(m);
}

void Engine::handleUCI()
{
    std::string strUCI;

    while (true)
    {
        getline(std::cin, strUCI);

        if (strUCI == "uci")
        {
            std::cout << "id name YaC-Engine\n";
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
            auto bestMove = goMCTS();
            std::cout << "\nbestmove " << board.to_string(bestMove) << "\n";
        }
        if (strUCI.rfind("position", 0) == 0)
        {
            strUCI = strUCI.substr(8);
            if (strUCI.find("startpos") != std::string::npos)
            {
                board.setInitialState();
                board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            }
            else if (strUCI.find("fen") != std::string::npos)
            {
                strUCI = strUCI.substr(strUCI.find("fen") + 4);
                board.setInitialState();
                board.setFENCode(strUCI.c_str());
            }
            strUCI = strUCI.substr(1);
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
                    if (!board.validateMove(&m))
                    {
                        std::cout << "Invalid move: " << board.to_string(m) << "\n";
                        continue;
                    }
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

Move Engine::goMCTS(int maxIterations)
{
    Moves moves;
    moves = board.generateMoves(moves);
    enginePlayColor = board.getColor();
    auto root = std::unique_ptr<Node>(new Node(Move(NULL_MOVE), moves, nullptr, board.getColor()));

    for (int i = 0; i < maxIterations; i++)
    {
        auto node = root.get();
        // selection
        while (node->moves.length == 0 && node->has_children())
        {
            node = node->selectChildWithMaxUCT();
            board.move(node->currentMove);
        }

        // expand node with one random move
        if (node->moves.length > 0)
        {
            auto randomMove = node->getRandomMove();
            board.move(randomMove);
            moves = board.generateMoves(moves);
            node = node->addChild(randomMove, moves, board.getColor());
        }

        // play a random game
        bool isMateFound = false;
        float value = 0.0f;
        float result = 0.0;

        // play a random game
        monteCarloSimulation(20, isMateFound, value);
        if (isMateFound)
        {
            if (value > 0.0f)
            {
                result = 1;
            }
            else
            {
                result = 0;
            }
        }

        // update wins back to the root
        while (node != nullptr)
        {
            if (node->parent != nullptr)
            {
                board.undoMove();
            }
            node->visits++;
            node->wins += result;
            node = node->parent;
        }
    }

    auto node = root.get();
    while (node->has_children())
    {
        sort(node->children.begin(), node->children.end(), [](Node *a, Node *b) {
            return a->wins > b->wins;
        });
        node = node->children[0];
    }

    /*std::cout << root.get()->tree_to_string();
    std::cout << "-------------------------";*/
    // std::cout << root.get()->tree_to_dot();

    return root.get()->children[0]->currentMove;
}

Move Engine::go(int searchDepth)
{
    Moves moves;
    std::vector<NodeStatistics> stats;

    monteCarloDepth = searchDepth;
    enginePlayColor = board.getColor();

    moves = board.generateMoves(moves);

    for (int i = 0; i < moves.length; i++)
    {
        stats.push_back(monteCarloSearch(board.getFENCode().c_str(), moves.move[i]));
    }
    std::cout << "\n";

    std::vector<float> results;
    std::vector<std::pair<float, int>> sortedResults;

    for (int i = 0; i < moves.length; i++)
    {
        float res = ((float)stats[i].win - (float)stats[i].lost) / (float)stats[i].trials;
        results.push_back(res);
        sortedResults.push_back(std::make_pair(results[i], i));
    }

    sort(sortedResults.begin(), sortedResults.end(), [](std::pair<float, int> a, std::pair<float, int> b) {
        return a.first > b.first;
    });

    for (int i = 0; i < sortedResults.size(); i++)
    {
        int index = sortedResults[i].second;
        std::cout << board.to_string(moves.move[index]) << " wins:" << stats[index].win
                  << " losts:" << stats[index].lost
                  << " trials:" << stats[index].trials << " -> " << results[index] << "\n";
    }

    board.move(moves.move[sortedResults[0].second]);
    return moves.move[sortedResults[0].second];
}

NodeStatistics Engine::monteCarloSearch(const char *fen, Move startingMove)
{
    float value = 0;
    bool isMateFound = false;

    NodeStatistics stat;

    board.move(startingMove);

    for (int i = 0; i < 1000; i++, stat.trials++)
    {
        monteCarloSimulation(monteCarloDepth, isMateFound, value);

        if (isMateFound)
        {
            if (value > 0.0f)
            {
                stat.win += value;
            }
            else
            {
                stat.lost += -value;
            }
            isMateFound = false;
            // std::cout << board.to_string(startingMove);
        }
    }

    board.undoMove();

    return stat;
}

void Engine::monteCarloSimulation(int depth, bool &isMateFound, float &value)
{
    uint64_t count = 0;
    Moves moves;

    moves = board.generateMoves(moves);

    // no moves then mate (or stalemate)
    if (moves.length == 0)
    {
        isMateFound = true;
        // std::cout << "\n"
        //          << board.getFENCode() << " " << (board.getColor() == WHITE ? "WHITE" : "BLACK") << "\n";
        value = (board.getColor() == enginePlayColor) ? -1.0f : 1.0f;
        return;
    }
    int randomIndex = rand() % moves.length;

    if (depth == 1)
    {
        return;
    }
    board.move(moves.move[randomIndex]);

    monteCarloSimulation(depth - 1, isMateFound, value);

    board.undoMove();

    if (isMateFound)
    {
        // std::cout << board.to_string(moves.move[randomIndex]) << " ";
    }

    return;
}

uint64_t Engine::perft(int depth)
{
    uint64_t count = 0;
    Moves moves;

    moves = board.generateMoves(moves);
    if (depth == 1)
    {
        /*
        for (int i = 0; i < moves.length; i++)
        {
            for (auto m : board.matchMoves)
            {
                board.fenFile << board.to_string(m) << '|';
            }
            board.fenFile << board.to_string(moves.move[i]) << "\n";
        }
        */
        return moves.length;
    }
    for (int i = 0; i < moves.length; i++)
    {
        board.move(moves.move[i]);
        board.matchMoves.push_back(moves.move[i]);
        count += perft(depth - 1);
        board.matchMoves.pop_back();
        board.undoMove();
    }
    return count;
}

void Engine::parseMove(std::string move, Move *m)
{
    m->start = (Position)(((move[1] - '1') << 3) + (move[0] - 'a'));
    m->end = (Position)(((move[3] - '1') << 3) + (move[2] - 'a'));
    // with promotion?
    if (move.length() == 5)
    {
        Color color = WHITE;
        if (move[3] == '1')
        {
            color = BLACK;
        }
        if (move[4] == 'r')
        {
            m->promotion = color == WHITE ? R : r;
        }
        if (move[4] == 'n')
        {
            m->promotion = color == WHITE ? N : n;
        }
        if (move[4] == 'b')
        {
            m->promotion = color == WHITE ? B : b;
        }
        if (move[4] == 'q')
        {
            m->promotion = color == WHITE ? Q : q;
        }
    }
}

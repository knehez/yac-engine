#pragma once

#include "util.h"
#include "chessboard.h"

struct NodeStatistics
{
    float win = 0;
    float lost = 0;
    uint32_t trials = 0;
};

class Engine
{
public:
    void start(void);
    uint64_t perft(int depth);
    void setFENCode(const char *fenstr);
    void move(Move m);
    Move go(int searchDepth = 12);
    Move goMCTS(int maxIterations = 10000);
private:
    void monteCarloSimulation(int depth, bool &isMateFound, float &value);
    void handleUCI();
    std::string gameMoves;
    void parseMove(std::string move, Move *m);
    NodeStatistics monteCarloSearch(const char *fen, Move startingMove);
    ChessBoard board;
    Color enginePlayColor;
    int monteCarloDepth;
};

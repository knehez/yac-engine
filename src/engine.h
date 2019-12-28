#pragma once

#include "util.h"
#include "chessboard.h"

class Engine
{
public:
    void start(void);
    uint64_t monteCarloSimulation(int depth, bool &isMateFound, int &value);
    uint64_t perft(int depth);
    void setFENCode(const char *fenstr);
private:
    void handleUCI();
    std::string gameMoves;
    void parseMove(std::string move, Move *m);
    Move go();
    int monteCarloSearch(const char *fen);
    ChessBoard board;
};

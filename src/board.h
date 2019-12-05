#pragma once
#include <string>
#include "constants.h"

class Board
{
public:
    Board();

    void setFENCode(std::string fenstr);
    void setPiece(int pos, char piece);
    
    std::string getFENCode();
    char getPiece(int pos);
    uint64_t getMaks(Position pos);
    uint64_t getWhitePiecesBoard();
    uint64_t getBlackPiecesBoard();

    uint64_t allPossibleMoves(int pos);
    uint64_t getBoard(char piece);

    uint64_t rankAttack(uint64_t occupancy, Position pos);
    uint64_t fileAttack(uint64_t occupancy, Position pos);
    uint64_t antidiagonalAttack(uint64_t pieces, Position x);
    uint64_t diagonalAttack(uint64_t pieces, Position x);
    
    void clear();

    std::string to_string(int startPos = a1, int endPos = h8);
    std::string showOneBitBoard(uint64_t board, int startPos = a1, int endPos = h8);
private:
    uint64_t attack(uint64_t pieces, Position pos, uint64_t mask);
    uint64_t knightMoves(int pos);
    uint64_t kingMoves(int pos);
    void trimWhiteSpaces(std::string &str);
    
    int generate_rank_attack(int o, int f);

    // DATA
    Mask MASK[h8];
    uint64_t m_boards[NumOfPieces];
    char m_pieces[NumOfPieces] = {'P', 'R', 'N', 'B', 'Q', 'K', 'p', 'r', 'n', 'b', 'q', 'k'};
    uint8_t RANK_ATTACK[512];
};
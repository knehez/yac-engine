#pragma once
#include <string>
#include "constants.h"
#include "gtest/gtest_prod.h"

class Board
{
public:
    Board();

    void setFENCode(std::string fenstr);
    std::string getFENCode();

    void setPiece(int pos, char piece);
    char getPiece(int pos);
    
    uint64_t getWhitePiecesBoard();
    uint64_t getBlackPiecesBoard();

    uint64_t getBoard(char piece);
    
    uint64_t allPossibleMoves(int pos);
    
    void clear();

    std::string to_string(int startPos = a1, int endPos = h8);
    std::string showOneBitBoard(uint64_t board, int startPos = a1, int endPos = h8);
private:
    FRIEND_TEST(boardTest, rankAttack);
    uint64_t rankAttack(uint64_t occupancy, Position pos);
    FRIEND_TEST(boardTest, fileAttack);
    uint64_t fileAttack(uint64_t occupancy, Position pos);
    FRIEND_TEST(boardTest, antiDiagonalAttack);
    uint64_t antidiagonalAttack(uint64_t pieces, Position x);
    FRIEND_TEST(boardTest, diagonalAttack);
    uint64_t diagonalAttack(uint64_t pieces, Position x);

    uint64_t attack(uint64_t pieces, Position pos, uint64_t mask);
    uint64_t knightMoves(int pos);
    uint64_t kingMoves(int pos);
    void trimWhiteSpaces(std::string &str);
    
    int generate_rank_attack(int o, int f);

    // DATA
    Mask MASK[h8];
    uint64_t m_boards[NumOfPieces];
    char *m_pieces = "PpRrNnBbQqKk";
    uint8_t RANK_ATTACK[512];
    Color color;
};
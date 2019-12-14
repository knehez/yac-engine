#pragma once
#include <string>
#include "constants.h"
#include "gtest/gtest_prod.h"


typedef struct move{
    Position start;
    Position end;
    Piece piece;
    Piece promotion = NUMBER_OF_PIECES;
} Move;

class Board
{
public:
    Board();

    void setFENCode(std::string fenstr);
    std::string getFENCode();

    void move(Move);
    void undoMove(Move);

    void setPiece(int pos, char piece);
    Piece getPieceAt(Position pos);
    Position getPiecePos(Piece piece);
    uint64_t getWhitePiecesBoard();
    uint64_t getBlackPiecesBoard();

    uint64_t getBoard(char piece);

    uint64_t allPieceMoves(int pos);
    std::string getMoves();
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
    uint64_t bishopMoves(int pos);
    uint64_t pawnMoves(int pos);
    uint64_t rookMoves(int pos);
    uint64_t pawnBlackMoves(int pos);
    uint64_t pawnBlackHitMoves(int pos);
    uint64_t pawnWhiteHitMoves(int pos);
    uint64_t pawnWhiteMoves(int pos);

    FRIEND_TEST(moveTest, whiteCheckers);
    uint64_t getCheckers();
    
    void trimWhiteSpaces(std::string &str);

    int generate_rank_attack(int o, int f);

    int nextSquare(uint64_t *board);
    
    // DATA
    Mask MASK[h8];
    uint64_t m_boards[NUMBER_OF_PIECES];
    uint8_t RANK_ATTACK[512];
    Color color;
};
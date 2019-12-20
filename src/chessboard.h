#pragma once

#include <string>
#include "util.h"
#include "gtest/gtest_prod.h"

typedef struct move
{
    Position start;
    Position end;
    Piece piece;
    Piece captured = NUMBER_OF_PIECES;
    Piece promotion = NUMBER_OF_PIECES;
    unsigned short castling = 0;
    Position enpassant = NUMBER_OF_SQUARES;
} Move;

class ChessBoard
{
public:
    ChessBoard();

    void setFENCode(const char* fenstr);
    std::string getFENCode();

    void move(Move);
    void undoMove(Move);

    void setPiece(int pos, char piece);
    Piece getPieceAt(Position pos);
    Position getPiecePos(Piece piece);
    uint64_t getWhitePiecesBoard();
    uint64_t getBlackPiecesBoard();

    uint64_t getBoard(char piece);

    uint64_t allPieceMoves(Position pos);
    std::string generateMoves();

    bool isMate();
    bool isStaleMate();
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

    Position nextSquare(uint64_t *board);

    uint64_t attack(uint64_t board, Position pos, uint64_t movesMask);
    uint64_t knightMoves(Position pos);
    uint64_t kingMoves(Position pos);
    uint64_t bishopMoves(Position pos);
    uint64_t pawnMoves(Position pos);
    uint64_t rookMoves(Position pos);
    uint64_t pawnBlackMoves(Position pos);
    uint64_t pawnBlackHitMoves(Position pos);
    uint64_t pawnWhiteHitMoves(Position pos);
    uint64_t pawnWhiteMoves(Position pos);

    FRIEND_TEST(moveTest, whiteCheckers);
    bool isSqareAttacked(Position square, uint64_t *board, Color oppositecolor);

    int generate_rank_attack(int o, int f);

    // DATA
    Mask MASK[NUMBER_OF_SQUARES];
    uint64_t m_boards[NUMBER_OF_PIECES];
    uint8_t RANK_ATTACK[512];
    Color color;

    FRIEND_TEST(boardTest, castlingFEN);
    FRIEND_TEST(moveTest, castling);
    // king and queen sides are enabled by default
    unsigned short castling = CASTLING_BLACK_KINGSIDE | CASTLING_BLACK_QUEENSIDE | CASTLING_WHITE_KINGSIDE | CASTLING_WHITE_QUEENSIDE;

    FRIEND_TEST(boardTest, entpassant);
    // enpassant position
    Position enpassant = NUMBER_OF_SQUARES;
};

#pragma once

#include <string>
#include "util.h"
#include "gtest/gtest_prod.h"
#include <vector>
#include <iostream>
#include <fstream>
#if __linux__
#include <string.h>
#endif
#include <stdlib.h>

struct BoardState
{
    Color color = WHITE;
    uint64_t boards[NUMBER_OF_PIECES];
    uint16_t castling = CASTLING_BLACK_KINGSIDE | CASTLING_BLACK_QUEENSIDE | CASTLING_WHITE_KINGSIDE | CASTLING_WHITE_QUEENSIDE;
    Position enpassant = NUMBER_OF_SQUARES;
};

class ChessBoard
{
public:
    ChessBoard();
    void initialState();

    std::ofstream fenFile;
    std::vector<Move> matchMoves;
    void setFENCode(const char *fenstr);
    std::string getFENCode();

    std::string to_string(Move move);
    std::string to_string(Position pos);
    std::string to_string(int startPos = a1, int endPos = h8);

    void move(Move);
    void undoMove(Move);
    bool validateMove(Move *);

    void setPiece(int pos, char piece);
    Piece getPieceAt(Position pos);
    Position getPiecePos(Piece piece);
    uint64_t getWhitePiecesBoard();
    uint64_t getBlackPiecesBoard();

    uint64_t getBoard(char piece);

    uint64_t allPieceMoves(Position pos);
    Moves generateMoves(Moves moves);

    bool isMate();
    bool isStaleMate();
    Color getColor();
    void clear();
    std::string showOneBitBoard(uint64_t board, int startPos = a1, int endPos = h8);
    //
    // misc - data
    //
    uint64_t perftCaptures = 0;
    uint64_t perftChecks = 0;

private:
    //restoreBoard();
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
    //
    // DATA - currentColor, pieces on the board
    //
    FRIEND_TEST(boardTest, castlingFEN);
    FRIEND_TEST(moveTest, castling1);
    FRIEND_TEST(moveTest, castling2);
    FRIEND_TEST(moveTest, castling3);
    FRIEND_TEST(moveTest, castling4);

    FRIEND_TEST(boardTest, entpassant);

    BoardState _stack[MAX_BOARD_MOVEMENTS], *state = &_stack[0];
    //
    // CONST DATA - does not change within a party
    //
    Mask MASK[NUMBER_OF_SQUARES];
    uint8_t RANK_ATTACK[512];
};

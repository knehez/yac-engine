#include "gtest/gtest.h"
#include "engine.h"

TEST(generalTest, MovesManipulation)
{
    Moves moves;
    moves.move[0] = Move({a7, a8, Q, p, a1});
    moves.move[1] = Move({b7, b8, Q, NUMBER_OF_PIECES, a1});
    moves.move[2] = Move({c7, c8, N, NUMBER_OF_PIECES, a2});
    moves.move[3] = Move({d7, d8, B, NUMBER_OF_PIECES, a3});
    moves.length = 4;

    EXPECT_EQ(ChessBoard::to_string(moves), "a7a8qb7b8qc7c8nd7d8b");
    
    removeOneMove(moves, Move({a7, a8, Q, p, a1}));
    
    EXPECT_EQ(ChessBoard::to_string(moves), "d7d8bb7b8qc7c8n");
    
    removeOneMove(moves, Move({d7, d8, B, NUMBER_OF_PIECES, a3}));
    
    EXPECT_EQ(ChessBoard::to_string(moves), "c7c8nb7b8q");
}
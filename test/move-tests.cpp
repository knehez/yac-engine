#include "gtest/gtest.h"
#include "board.h"

TEST(moveTest, initialPositionWhite)
{
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");

    auto moves = board.getMoves();
    EXPECT_EQ(moves, "a7a5|a7a6|b7b5|b7b6|c7c5|c7c6|d7d5|d7d6|e7e5|e7e6|f7f5|f7f6|g7g5|g7g6|h7h5|h7h6|b8a6|b8c6|g8f6|g8h6|");
}

TEST(moveTest, initialPositionBlack)
{
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    auto moves = board.getMoves();
    EXPECT_EQ(moves, "b1a3|b1c3|g1f3|g1h3|a2a3|a2a4|b2b3|b2b4|c2c3|c2c4|d2d3|d2d4|e2e3|e2e4|f2f3|f2f4|g2g3|g2g4|h2h3|h2h4|");
}

TEST(moveTest, whiteCheckers)
{
    Board board;
    board.setFENCode("1Q3B2/8/R2k4/2P1PN2/8/8/8/8 b - - 0 1");

    auto b = board.getCheckers();

    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "-X---X--");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "X-------");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--X-XX--");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");
}

TEST(moveTest, escapeFromCheck)
{
    Board board;
    board.setFENCode("1Q3B2/8/R2k4/2P1PN2/8/8/8/8 b - - 0 1");
    auto moves = board.getMoves();
    EXPECT_EQ(moves, "d6d5|d6d7|");
}

TEST(moveTest, escapeFromCheck2)
{
    Board board;
    board.setFENCode("K7/PN6/8/3b4/8/8/8/3k4 w - - 0 1");
    auto moves = board.getMoves();
    EXPECT_EQ(moves, "a8b8|");
}

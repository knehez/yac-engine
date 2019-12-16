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

    auto b = board.isSqareAttacked(board.getPiecePos(k), &board.m_boards[0], oppositeColor(BLACK));

    EXPECT_EQ(b, true);
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

TEST(moveTest, castling)
{
    Board board;
    board.setFENCode("r3k1nr/pppppppp/8/8/8/8/PPPPPPPP/R1B1K2R w KQkq - 0 1");
    auto moves = board.getMoves();
    // castling e1g1 is possible
    EXPECT_EQ(moves, "a1b1|e1d1|e1f1|h1f1|h1g1|a2a3|a2a4|b2b3|b2b4|c2c3|c2c4|d2d3|d2d4|e2e3|e2e4|f2f3|f2f4|g2g3|g2g4|h2h3|h2h4|e1g1|");

    board.setFENCode("r3k1nr/pppppppp/8/8/8/8/PPPPPPPP/R1B1K2R b KQkq - 0 1");
    moves = board.getMoves();
    // castling e8g8 is possible
    EXPECT_EQ(moves, "a7a5|a7a6|b7b5|b7b6|c7c5|c7c6|d7d5|d7d6|e7e5|e7e6|f7f5|f7f6|g7g5|g7g6|h7h5|h7h6|a8b8|a8c8|a8d8|e8d8|e8f8|g8f6|g8h6|e8c8|");
}

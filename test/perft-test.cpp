#include "gtest/gtest.h"
#include "chessboard.h"

TEST(perftTest, perftInitial)
{
    ChessBoard board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(board.perft(2), 400);
}

TEST(perftTest, perftDoubleCheck)
{
    ChessBoard board;
    board.setFENCode("8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1");
    EXPECT_EQ(board.perft(4), 23527);
}

TEST(perftTest, StaleMateCheckMate)
{
    ChessBoard board;
    board.setFENCode("8/k1P5/8/1K6/8/8/8/8 w - - 0 1");
    EXPECT_EQ(board.perft(7), 567584);
}

TEST(perftTest, SelfStaleMate)
{
    ChessBoard board;
    board.setFENCode("K1k5/8/P7/8/8/8/8/8 w - - 0 1");
    EXPECT_EQ(board.perft(6), 2217);
}

TEST(perftTest, PromotionGivesCheck)
{
    ChessBoard board;
    board.setFENCode("4k3/1P6/8/8/8/8/K7/8 w - - 0 1");
    EXPECT_EQ(board.perft(6), 217342);
}

TEST(perftTest, UnderPromotionGivesCheck)
{
    ChessBoard board;
    board.setFENCode("8/P1k5/K7/8/8/8/8/8 w - - 0 1");
    EXPECT_EQ(board.perft(6), 92683);
}

TEST(perftTest, generalPerft)
{
    ChessBoard board;
    board.setFENCode("rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 6");
    //EXPECT_EQ(board.perft(1), 42);
    //EXPECT_EQ(board.perft(2), 1352);
    EXPECT_EQ(board.perft(3), 53392);
}

TEST(perftTest, DiscoveredCheck)
{
    ChessBoard board;
    board.setFENCode("8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1");
    EXPECT_EQ(board.perft(5), 1004658);
}

TEST(perftTest, ShortCastlingCheck)
{
    ChessBoard board;
    board.setFENCode("5k2/8/8/8/8/8/8/4K2R w K - 0 1");
    EXPECT_EQ(board.perft(6), 661072);
}

TEST(perftTest, PromoteOutOfCheck)
{
    ChessBoard board;
    board.setFENCode("2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1");
    EXPECT_EQ(board.perft(6), 3821001);
}

TEST(perftTest, EnPassasantCaptureCheck)
{
    ChessBoard board;
    board.setFENCode("8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1");

    EXPECT_EQ(board.perft(6), 1440467);
}



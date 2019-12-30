#include "gtest/gtest.h"
#include "engine.h"

TEST(perftTest, perftInitial)
{
    Engine engine;
    engine.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(engine.perft(2), 400);
}

TEST(perftTest, perftDoubleCheck)
{
    Engine engine;
    engine.setFENCode("8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1");
    EXPECT_EQ(engine.perft(4), 23527);
}

TEST(perftTest, StaleMateCheckMate)
{
    Engine engine;
    engine.setFENCode("8/k1P5/8/1K6/8/8/8/8 w - - 0 1");
    EXPECT_EQ(engine.perft(7), 567584);
}

TEST(perftTest, SelfStaleMate)
{
    Engine engine;
    engine.setFENCode("K1k5/8/P7/8/8/8/8/8 w - - 0 1");
    EXPECT_EQ(engine.perft(6), 2217);
}

TEST(perftTest, PromotionGivesCheck)
{
    Engine engine;
    engine.setFENCode("4k3/1P6/8/8/8/8/K7/8 w - - 0 1");
    EXPECT_EQ(engine.perft(6), 217342);
}

TEST(perftTest, UnderPromotionGivesCheck)
{
    Engine engine;
    engine.setFENCode("8/P1k5/K7/8/8/8/8/8 w - - 0 1");
    EXPECT_EQ(engine.perft(6), 92683);
}

TEST(perftTest, generalPerft)
{
    Engine engine;
    engine.setFENCode("rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 6");
    //EXPECT_EQ(engine.perft(1), 42);
    //EXPECT_EQ(engine.perft(2), 1352);
    EXPECT_EQ(engine.perft(3), 53392);
}

TEST(perftTest, DiscoveredCheck)
{
    Engine engine;
    engine.setFENCode("8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1");
    EXPECT_EQ(engine.perft(5), 1004658);
}

TEST(perftTest, ShortCastlingCheck)
{
    Engine engine;
    engine.setFENCode("5k2/8/8/8/8/8/8/4K2R w K - 0 1");
    EXPECT_EQ(engine.perft(6), 661072);
}

TEST(perftTest, PromoteOutOfCheck)
{
    Engine engine;
    engine.setFENCode("2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1");
    EXPECT_EQ(engine.perft(6), 3821001);
}

TEST(perftTest, EnPassasantCaptureCheck)
{
    Engine engine;
    engine.setFENCode("8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1");

    EXPECT_EQ(engine.perft(6), 1440467);
}

TEST(perftTest, EnPassasantCaptureCheck2)
{
    Engine engine;
    engine.setFENCode("8/5bk1/8/2Pp4/8/1K6/8/8 w - d6 0 1");

    EXPECT_EQ(engine.perft(6), 824064);
}

TEST(perftTest, GeneralPerft2)
{
    Engine engine;
    engine.setFENCode("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    EXPECT_EQ(engine.perft(4), 4085603);
}

TEST(perftTest, discoverPromotinBug)
{
    Engine engine;
    engine.setFENCode("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");

    EXPECT_EQ(engine.perft(5), 3605103);
}

TEST(perftTest, GeneralPerftSet)
{
    Engine engine;
    engine.setFENCode("3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1");
    EXPECT_EQ(engine.perft(6), 1134888);
    engine.setFENCode("3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1");
    EXPECT_EQ(engine.perft(6), 1134888);
    engine.setFENCode("8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1");
    EXPECT_EQ(engine.perft(6), 1015133);
    engine.setFENCode("8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1");
    EXPECT_EQ(engine.perft(6), 1440467);
    engine.setFENCode("5k2/8/8/8/8/8/8/4K2R w K - 0 1");
    EXPECT_EQ(engine.perft(6), 661072);
    engine.setFENCode("3k4/8/8/8/8/8/8/R3K3 w Q - 0 1");
    EXPECT_EQ(engine.perft(6), 803711);
    engine.setFENCode("r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1");
    EXPECT_EQ(engine.perft(4), 1274206);
    engine.setFENCode("r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1");
    EXPECT_EQ(engine.perft(4), 1720476);
    //--Promote out of Check
    engine.setFENCode("2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1");
    EXPECT_EQ(engine.perft(6), 3821001);
    //--Discovered Check
    engine.setFENCode("8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1");
    EXPECT_EQ(engine.perft(5), 1004658);
    //--Promote to give check
    engine.setFENCode("4k3/1P6/8/8/8/8/K7/8 w - - 0 1");
    EXPECT_EQ(engine.perft(6), 217342);
    //--Under Promote to give check
    engine.setFENCode("8/P1k5/K7/8/8/8/8/8 w - - 0 1");
    EXPECT_EQ(engine.perft(6), 92683);
    //--Self Stalemate
    engine.setFENCode("K1k5/8/P7/8/8/8/8/8 w - - 0 1");
    EXPECT_EQ(engine.perft(6), 2217);
    //--Stalemate & Checkmate
    engine.setFENCode("8/k1P5/8/1K6/8/8/8/8 w - - 0 1");
    EXPECT_EQ(engine.perft(7), 567584);
    //--Stalemate & Checkmate
    engine.setFENCode("8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1");
    EXPECT_EQ(engine.perft(4), 23527);
    testing::internal::CaptureStdout();
    std::string output = testing::internal::GetCapturedStdout();
}

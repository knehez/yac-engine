#include "gtest/gtest.h"
#include "engine.h"

TEST(gameTest, MateInTwo)
{
    srand(0); // make random moves determnistic
    Engine engine;
    engine.setFENCode("2kr2nr/pp1n1ppp/2p1p3/8/1P1P1B2/2N2Q1P/1PPKBPP1/7q w - - 0 1");
    Move bestMove = engine.go();

    EXPECT_EQ(ChessBoard::to_string(bestMove), "e2a6");
}

TEST(gameTest, MateInSix)
{
    srand(0); // make random moves deterministic
    Engine engine;
    engine.setFENCode("k7/pp1r4/1n3b2/1N6/5Qp1/5pPq/5P2/4R1K1 w - - 0 1");
    Move bestMove = engine.go(2);

    EXPECT_EQ(true, true);
}

TEST(gameTest, MCTSTest1)
{
    srand(0); // make random moves deterministic
    Engine engine;
    engine.setFENCode("k7/pp1r2p1/1n3b2/pN1p2p1/P1pP1Qp1/2P2pPq/5P2/4R1K1 w - - 0 1");
    Move bestMove = engine.goMCTS(10000);

    EXPECT_EQ(ChessBoard::to_string(bestMove), "e1e8");
}

TEST(gameTest, MateInSix2)
{
    srand(0); // make random moves deterministic
    Engine engine;
    engine.setFENCode("r4k2/pp1P2pp/1q6/3Q2p1/2p5/8/5PPP/4R1K1 w - - 0 1");
                       
    Move bestMove = engine.goMCTS(10000);
    
    EXPECT_EQ(true, true);
    // EXPECT_EQ(ChessBoard::to_string(bestMove), "e1e8");
}

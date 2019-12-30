#include "gtest/gtest.h"
#include "engine.h"

TEST(gameTest, MateInTwo)
{
    srand(0); // make random moves determnistic
    Engine engine;
    engine.setFENCode("2kr2nr/pp1n1ppp/2p1p3/8/1P1P1B2/2N2Q1P/1PPKBPP1/7q w - - 0 1");
    Move bestMove = engine.go();

    EXPECT_EQ(ChessBoard::to_string(bestMove), "f3c6");
}

TEST(gameTest, MateInSix)
{
    srand(0); // make random moves determnistic
    Engine engine;
    engine.setFENCode("k7/pp1r4/1n3b2/1N6/5Qp1/5pPq/5P2/4R1K1 w - - 0 1");
    Move bestMove = engine.go(2);

    EXPECT_EQ(true, true);
}

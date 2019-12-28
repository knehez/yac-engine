#include "gtest/gtest.h"
#include "chessboard.h"

TEST(moveTest, initialPositionBlack)
{
    ChessBoard board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");

    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "a7a5|a7a6|b7b5|b7b6|c7c5|c7c6|d7d5|d7d6|e7e5|e7e6|f7f5|f7f6|g7g5|g7g6|h7h5|h7h6|b8a6|b8c6|g8f6|g8h6|");
}

TEST(moveTest, initialPositionWhite)
{
    ChessBoard board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "b1a3|b1c3|g1f3|g1h3|a2a3|a2a4|b2b3|b2b4|c2c3|c2c4|d2d3|d2d4|e2e3|e2e4|f2f3|f2f4|g2g3|g2g4|h2h3|h2h4|");
}

TEST(moveTest, whiteCheckers)
{
    ChessBoard board;
    board.setFENCode("1Q3B2/8/R2k4/2P1PN2/8/8/8/8 b - - 0 1");

    auto b = board.isSqareAttacked(board.getPiecePos(k), &board.state->boards[0], oppositeColor(BLACK));

    EXPECT_EQ(b, true);
}

TEST(moveTest, escapeFromCheck)
{
    ChessBoard board;
    board.setFENCode("1Q3B2/8/R2k4/2P1PN2/8/8/8/8 b - - 0 1");
    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "d6d5|d6d7|");
}

TEST(moveTest, escapeFromCheck2)
{
    ChessBoard board;
    board.setFENCode("K7/PN6/8/3b4/8/8/8/3k4 w - - 0 1");
    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "a8b8|");
}

TEST(moveTest, castling1)
{
    ChessBoard board;
    board.setFENCode("r3k1nr/pppppppp/8/8/8/8/PPPPPPPP/R1B1K2R w KQkq - 0 1");
    Moves m;
    auto moves = board.generateMoves(m);
    // castling e1g1 is possible
    EXPECT_EQ(to_string(moves), "a1b1|e1d1|e1f1|h1f1|h1g1|a2a3|a2a4|b2b3|b2b4|c2c3|c2c4|d2d3|d2d4|e2e3|e2e4|f2f3|f2f4|g2g3|g2g4|h2h3|h2h4|e1g1|");
}

TEST(moveTest, castling2)
{
    ChessBoard board;
    board.setFENCode("r3k1nr/pppppppp/8/8/8/8/PPPPPPPP/R1B1K2R b KQkq - 0 1");
    Moves m;
    auto moves = board.generateMoves(m);
    // castling e8g8 is possible
    EXPECT_EQ(to_string(moves), "a7a5|a7a6|b7b5|b7b6|c7c5|c7c6|d7d5|d7d6|e7e5|e7e6|f7f5|f7f6|g7g5|g7g6|h7h5|h7h6|a8b8|a8c8|a8d8|e8d8|e8f8|g8f6|g8h6|e8c8|");
}

TEST(moveTest, castling3)
{
    ChessBoard board;
    board.setFENCode("rn2k2r/pppppppp/8/8/8/8/PPPPPPPP/R3KB1R b KQkq");
    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "a7a5|a7a6|b7b5|b7b6|c7c5|c7c6|d7d5|d7d6|e7e5|e7e6|f7f5|f7f6|g7g5|g7g6|h7h5|h7h6|b8a6|b8c6|e8d8|e8f8|h8f8|h8g8|e8g8|");
}

TEST(moveTest, castling4)
{
    ChessBoard board;
    board.setFENCode("rn2k2r/pppppppp/8/8/8/8/PPPPPPPP/R3KB1R w KQkq");
    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "a1b1|a1c1|a1d1|e1d1|h1g1|a2a3|a2a4|b2b3|b2b4|c2c3|c2c4|d2d3|d2d4|e2e3|e2e4|f2f3|f2f4|g2g3|g2g4|h2h3|h2h4|e1c1|");
}

TEST(moveTest, moveUndoMove)
{
    const char *fenCode = "3k4/4b1p1/1Qp5/8/1R1b4/8/6P1/3K4 b - - 0 1";
    ChessBoard board;
    board.setFENCode(fenCode);

    Move m1;
    Move m2;

    m1.start = d4;
    m1.end = b6;
    m1.captured = Q;

    m2.start = b4;
    m2.end = b6;
    m2.captured = b;

    board.move(m1);
    board.move(m2);
    board.undoMove(m2);
    board.undoMove(m1);
    EXPECT_EQ(board.getFENCode(), fenCode);
}

TEST(moveTest, moreMoves)
{
    const char *fenCode = "rn1qkb1r/pp3ppp/2p1pnb1/6B1/1P2N3/3P1N2/P1P1QPPP/R3KB1R b KQkq - 0 8";
    ChessBoard board;
    board.setFENCode(fenCode);

    Move m1;
    Move m2;
    m1.start = f6;
    m1.end = e4;
    m1.captured = N;

    m2.start = d3;
    m2.end = e4;
    m2.captured = n;

    board.move(m1);
    board.move(m2);
    board.undoMove(m2);
    board.undoMove(m1);

    EXPECT_EQ(board.getFENCode(), "rn1qkb1r/pp3ppp/2p1pnb1/6B1/1P2N3/3P1N2/P1P1QPPP/R3KB1R b KQkq - 0 1");
}

TEST(moveTest, blackPromotionsSimpleUndo)
{
    ChessBoard board;
    board.setFENCode("8/1P6/5k2/8/8/5K2/1p6/8 b - - 0 1");
    Move m1;

    m1.start = b2;
    m1.end = b1;

    m1.promotion = r;
    board.move(m1);
    board.undoMove(m1);
    // undo() do not make wrong the boards
    EXPECT_EQ(board.getFENCode(), "8/1P6/5k2/8/8/5K2/1p6/8 b - - 0 1");
}

TEST(moveTest, blackAllPromotionsSimple)
{
    ChessBoard board;
    board.setFENCode("8/1P6/5k2/8/8/5K2/1p6/8 b - - 0 1");
    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "b2b1(r)|b2b1(n)|b2b1(b)|b2b1(q)|f6e5|f6f5|f6g5|f6e6|f6g6|f6e7|f6f7|f6g7|");
    // undo() do not make wrong the boards
    EXPECT_EQ(board.getFENCode(), "8/1P6/5k2/8/8/5K2/1p6/8 b - - 0 1");
}

TEST(moveTest, whiteAllPromotionsSimple)
{
    ChessBoard board;
    board.setFENCode("8/1P6/5k2/8/8/5K2/1p6/8 w - - 0 1");
    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "f3e2|f3f2|f3g2|f3e3|f3g3|f3e4|f3f4|f3g4|b7b8(R)|b7b8(N)|b7b8(B)|b7b8(Q)|");
    // undo() do not make wrong the boards
    EXPECT_EQ(board.getFENCode(), "8/1P6/5k2/8/8/5K2/1p6/8 w - - 0 1");
}

TEST(moveTest, enPassantSimpleBlack)
{
    ChessBoard board;
    board.setFENCode("8/8/1k6/8/3Pp3/8/8/4K3 b - d3 0 1");
    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "e4d3|e4e3|b6a5|b6b5|b6a6|b6c6|b6a7|b6b7|b6c7|");
    EXPECT_EQ(board.getFENCode(), "8/8/1k6/8/3Pp3/8/8/4K3 b - d3 0 1");
}

TEST(moveTest, enPassantSimpleWhite)
{
    ChessBoard board;
    board.setFENCode("8/8/8/3Pp3/5K2/8/k7/8 w - e6 0 1");
    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "f4e3|f4f3|f4g3|f4e4|f4g4|f4e5|f4f5|f4g5|d5e6|");
}

TEST(moveTest, enPassantCheckBlack)
{
    ChessBoard board;
    board.setFENCode("8/8/8/1k6/3Pp3/8/8/4KQ2 b - d3 0 1");
    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "e4d3|b5a4|b5b4|b5a5|b5b6|b5c6|");
    // undo() do not make wrong the boards
    EXPECT_EQ(board.getFENCode(), "8/8/8/1k6/3Pp3/8/8/4KQ2 b - d3 0 1");
}

TEST(moveTest, SetEnpassantFlag)
{
    ChessBoard board;
    board.setFENCode("k7/8/8/8/1p6/8/P7/K7 w - - 0 1");
    Move m1;
    m1.start = a2;
    m1.end = a4;

    board.move(m1);

    Moves m;
    auto moves = board.generateMoves(m);
    // enpassant must be part of the moves -> b4a3
    EXPECT_EQ(to_string(moves), "b4a3|b4b3|a8a7|a8b7|a8b8|");
}

TEST(moveTest, isInMate)
{
    ChessBoard board;
    board.setFENCode("R3k3/8/4K3/8/8/8/8/8 b - - 0 1");

    EXPECT_EQ(board.isMate(), true);
}

TEST(moveTest, isInStaleMate)
{
    ChessBoard board;
    board.setFENCode("8/8/8/8/3b4/5k1p/7P/7K w - - 0 1");

    EXPECT_EQ(board.isStaleMate(), true);

    // similar situation with black
    board.setFENCode("k7/p2K4/N7/2N5/8/8/8/8 b - - 0 1");
    EXPECT_EQ(board.isStaleMate(), true);
}

TEST(moveTest, doubleChecksBlack)
{
    ChessBoard board;
    board.setFENCode("3nk3/3p2N1/5b2/4R3/2B5/8/8/3K4 b - - 0 1");

    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "e8f8|");
}

TEST(moveTest, pinnedRook)
{
    ChessBoard board;
    board.setFENCode("4k3/8/4r3/8/4Q3/8/8/3K4 b - - 0 1");

    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "e6e4|e6e5|e6e7|e8d7|e8e7|e8f7|e8d8|e8f8|");
}

TEST(moveTest, pawnBugFix)
{
    ChessBoard board;
    board.setFENCode("rnbqkbnr/1ppppppp/8/p7/8/7P/PPPPPPP1/RNBQKBNR w - - 0 3");

    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "b1a3|b1c3|g1f3|h1h2|a2a3|a2a4|b2b3|b2b4|c2c3|c2c4|d2d3|d2d4|e2e3|e2e4|f2f3|f2f4|g2g3|g2g4|h3h4|");
}

TEST(moveTest, pawnBugFix2)
{
    ChessBoard board;
    board.setFENCode("r1bqkbnr/pppppppp/2n5/7P/8/8/PPPPPPP1/RNBQKBNR b - - 0 4");
    Moves m;
    auto moves = board.generateMoves(m);
    EXPECT_EQ(to_string(moves), "c6b4|c6d4|c6a5|c6e5|c6b8|a7a5|a7a6|b7b5|b7b6|d7d5|d7d6|e7e5|e7e6|f7f5|f7f6|g7g5|g7g6|h7h6|a8b8|g8f6|g8h6|");
}

TEST(moveTest, castlingBug)
{
    ChessBoard board;
    board.setFENCode("5k2/8/8/8/8/7p/7P/4K2R w K - 0 1");
    Move m1;
    m1.start = e1;
    m1.end = g1;

    Move m2;
    m2.start = f8;
    m2.end = e8;

    Move m3;
    m3.start = f1;
    m3.end = e1;
    board.move(m1);
    board.move(m2);
    board.move(m3);

    EXPECT_EQ(board.getFENCode(), "4k3/8/8/8/8/7p/7P/4R1K1 b - - 0 1");

    board.undoMove(m3);
    board.undoMove(m2);
    board.undoMove(m1);
    EXPECT_EQ(board.getFENCode(), "5k2/8/8/8/8/7p/7P/4K2R w K - 0 1");
}

TEST(moveTest, castlingBug2)
{
    ChessBoard board;
    board.setFENCode("5k2/8/8/8/8/7p/7P/4K2R w K - 0 1");
    Move m1;
    m1.start = e1;
    m1.end = g1;

    board.move(m1);
    Moves m;
    auto moves = board.generateMoves(m);

    EXPECT_EQ(to_string(moves), "f8e7|f8g7|f8e8|f8g8|");
}

TEST(moveTest, enpassantMissingBug)
{
    ChessBoard board;
    board.setFENCode("rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 6");
    Move m1;
    m1.start = e1;
    m1.end = d2;

    Move m2;
    m2.start = f7;
    m2.end = f5;
    m2.enpassant = f6;

    board.move(m1);
    board.move(m2);

    Moves m;
    auto moves = board.generateMoves(m);

    EXPECT_EQ(to_string(moves), "b1a3|b1c3|d1e1|d1f1|d1g1|h1e1|h1f1|h1g1|a2a3|a2a4|b2b3|b2b4|c2c3|d2e1|d2c3|d2e3|e2g1|e2c3|e2g3|e2d4|e2f4|g2g3|g2g4|h2h3|h2h4|c4b3|c4d3|c4b5|c4d5|c4a6|c4e6|c4f7|c4g8|e5e6|e5f6|");
}

TEST(moveTest, wrongPawnmoveBug)
{
    ChessBoard board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Move m1;
    m1.start = h2;
    m1.end = h4;

    Move m2;
    m2.start = a7;
    m2.end = a5;

    board.move(m1);
    board.move(m2);

    Moves m;
    auto moves = board.generateMoves(m);

    EXPECT_EQ(to_string(moves), "b1a3|b1c3|g1f3|g1h3|h1h2|h1h3|a2a3|a2a4|b2b3|b2b4|c2c3|c2c4|d2d3|d2d4|e2e3|e2e4|f2f3|f2f4|g2g3|g2g4|h4h5|");
}

TEST(moveTest, PromotionAndHitBug)
{
    ChessBoard board;
    board.setFENCode("8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1");

    Move m1;
    m1.start = b3;
    m1.end = a3;
    board.move(m1);

    m1.start = b6;
    m1.end = b7;
    board.move(m1);

    m1.start = a3;
    m1.end = a8;
    board.move(m1);

    m1.start = b7;
    m1.end = a8;
    m1.promotion = R;
    m1.captured = q;
    board.move(m1);

    Moves m;
    Moves moves;
    moves = board.generateMoves(m);

    EXPECT_EQ(to_string(moves), "f1e1|f1g1|f1e2|f1f2|f1g2|c4b2|c4d2|c4a3|c4e3|c4a5|c4e5|c4b6|c4d6|");
}


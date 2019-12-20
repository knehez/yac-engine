#include "gtest/gtest.h"
#include "chessboard.h"

TEST(moveTest, initialPositionWhite)
{
    ChessBoard board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");

    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "a7a5|a7a6|b7b5|b7b6|c7c5|c7c6|d7d5|d7d6|e7e5|e7e6|f7f5|f7f6|g7g5|g7g6|h7h5|h7h6|b8a6|b8c6|g8f6|g8h6|");
}

TEST(moveTest, initialPositionBlack)
{
    ChessBoard board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "b1a3|b1c3|g1f3|g1h3|a2a3|a2a4|b2b3|b2b4|c2c3|c2c4|d2d3|d2d4|e2e3|e2e4|f2f3|f2f4|g2g3|g2g4|h2h3|h2h4|");
}

TEST(moveTest, whiteCheckers)
{
    ChessBoard board;
    board.setFENCode("1Q3B2/8/R2k4/2P1PN2/8/8/8/8 b - - 0 1");

    auto b = board.isSqareAttacked(board.getPiecePos(k), &board.m_boards[0], oppositeColor(BLACK));

    EXPECT_EQ(b, true);
}

TEST(moveTest, escapeFromCheck)
{
    ChessBoard board;
    board.setFENCode("1Q3B2/8/R2k4/2P1PN2/8/8/8/8 b - - 0 1");
    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "d6d5|d6d7|");
}

TEST(moveTest, escapeFromCheck2)
{
    ChessBoard board;
    board.setFENCode("K7/PN6/8/3b4/8/8/8/3k4 w - - 0 1");
    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "a8b8|");
}

TEST(moveTest, castling)
{
    ChessBoard board;
    board.setFENCode("r3k1nr/pppppppp/8/8/8/8/PPPPPPPP/R1B1K2R w KQkq - 0 1");
    auto moves = board.generateMoves();
    // castling e1g1 is possible
    EXPECT_EQ(moves, "a1b1|e1d1|e1f1|h1f1|h1g1|a2a3|a2a4|b2b3|b2b4|c2c3|c2c4|d2d3|d2d4|e2e3|e2e4|f2f3|f2f4|g2g3|g2g4|h2h3|h2h4|e1g1|");
    EXPECT_EQ(board.castling, CASTLING_BLACK_KINGSIDE | CASTLING_BLACK_QUEENSIDE);
    EXPECT_EQ(board.castling, board.castling & ~(CASTLING_WHITE_KINGSIDE | CASTLING_WHITE_QUEENSIDE));
    board.setFENCode("r3k1nr/pppppppp/8/8/8/8/PPPPPPPP/R1B1K2R b KQkq - 0 1");
    moves = board.generateMoves();
    // castling e8g8 is possible
    EXPECT_EQ(moves, "a7a5|a7a6|b7b5|b7b6|c7c5|c7c6|d7d5|d7d6|e7e5|e7e6|f7f5|f7f6|g7g5|g7g6|h7h5|h7h6|a8b8|a8c8|a8d8|e8d8|e8f8|g8f6|g8h6|e8c8|");
    EXPECT_EQ(board.castling, board.castling & ~(CASTLING_BLACK_KINGSIDE | CASTLING_BLACK_QUEENSIDE));

    board.setFENCode("rn2k2r/pppppppp/8/8/8/8/PPPPPPPP/R3KB1R b KQkq");
    moves = board.generateMoves();
    EXPECT_EQ(moves, "a7a5|a7a6|b7b5|b7b6|c7c5|c7c6|d7d5|d7d6|e7e5|e7e6|f7f5|f7f6|g7g5|g7g6|h7h5|h7h6|b8a6|b8c6|e8d8|e8f8|h8f8|h8g8|e8g8|");
    EXPECT_EQ(board.castling, board.castling & ~(CASTLING_BLACK_KINGSIDE | CASTLING_BLACK_QUEENSIDE));

    board.setFENCode("rn2k2r/pppppppp/8/8/8/8/PPPPPPPP/R3KB1R w KQkq");
    moves = board.generateMoves();
    EXPECT_EQ(moves, "a1b1|a1c1|a1d1|e1d1|h1g1|a2a3|a2a4|b2b3|b2b4|c2c3|c2c4|d2d3|d2d4|e2e3|e2e4|f2f3|f2f4|g2g3|g2g4|h2h3|h2h4|e1c1|");
    EXPECT_EQ(board.castling, board.castling & ~(CASTLING_WHITE_KINGSIDE | CASTLING_WHITE_QUEENSIDE));
}

TEST(moveTest, moveUndoMove)
{
    const char *fenCode = "3k4/4b1p1/1Qp5/8/1R1b4/8/6P1/3K4";
    ChessBoard board;
    board.setFENCode(fenCode);

    Move m1;
    Move m2;

    m1.start = d4;
    m1.end = b6;
    m1.piece = b;
    m1.captured = Q;

    m2.start = b4;
    m2.end = b6;
    m2.piece = R;
    m2.captured = b;

    board.move(m1);
    board.move(m2);
    board.undoMove(m2);
    board.undoMove(m1);
    EXPECT_EQ(board.getFENCode(), fenCode);
}

TEST(moveTest, blackPromotionsSimpleUndo)
{
    ChessBoard board;
    board.setFENCode("8/1P6/5k2/8/8/5K2/1p6/8 b - - 0 1");
    Move m1;

    m1.start = b2;
    m1.end = b1;
    m1.piece = p;
    m1.captured = NUMBER_OF_PIECES;
    m1.promotion = r;
    board.move(m1);
    board.undoMove(m1);
    // undo() do not make wrong the boards
    EXPECT_EQ(board.getFENCode(), "8/1P6/5k2/8/8/5K2/1p6/8");
}

TEST(moveTest, blackAllPromotionsSimple)
{
    ChessBoard board;
    board.setFENCode("8/1P6/5k2/8/8/5K2/1p6/8 b - - 0 1");
    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "b2b1(r)|b2b1(n)|b2b1(b)|b2b1(q)|f6e5|f6f5|f6g5|f6e6|f6g6|f6e7|f6f7|f6g7|");
    // undo() do not make wrong the boards
    EXPECT_EQ(board.getFENCode(), "8/1P6/5k2/8/8/5K2/1p6/8");
}

TEST(moveTest, whiteAllPromotionsSimple)
{
    ChessBoard board;
    board.setFENCode("8/1P6/5k2/8/8/5K2/1p6/8 w - - 0 1");
    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "f3e2|f3f2|f3g2|f3e3|f3g3|f3e4|f3f4|f3g4|b7b8(R)|b7b8(N)|b7b8(B)|b7b8(Q)|");
    // undo() do not make wrong the boards
    EXPECT_EQ(board.getFENCode(), "8/1P6/5k2/8/8/5K2/1p6/8");
}

TEST(moveTest, enPassantSimpleBlack)
{
    ChessBoard board;
    board.setFENCode("8/8/8/2k5/3Pp3/8/8/4K3 b - d3 0 1");
    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "e4d3|c5b4|c5c4|c5d4|c5b5|c5d5|c5b6|c5c6|c5d6|");
    // undo() do not make wrong the boards
    EXPECT_EQ(board.getFENCode(), "8/8/8/2k5/3Pp3/8/8/4K3");
}

TEST(moveTest, enPassantSimpleWhite)
{
    ChessBoard board;
    board.setFENCode("8/8/8/3Pp3/5K2/8/k7/8 w - e6 0 1");
    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "f4e3|f4f3|f4g3|f4e4|f4g4|f4e5|f4f5|f4g5|d5e6|");
    // undo() do not make wrong the boards
    EXPECT_EQ(board.getFENCode(), "8/8/8/3Pp3/5K2/8/k7/8");
}

TEST(moveTest, enPassantCheckBlack)
{
    ChessBoard board;
    board.setFENCode("8/8/8/1k6/3Pp3/8/8/4KQ2 b - d3 0 1");
    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "e4d3|b5a4|b5b4|b5a5|b5b6|b5c6|");
    // undo() do not make wrong the boards
    EXPECT_EQ(board.getFENCode(), "8/8/8/1k6/3Pp3/8/8/4KQ2");
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

    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "e8f8|");
}

TEST(moveTest, pinnedRook)
{
    ChessBoard board;
    board.setFENCode("4k3/8/4r3/8/4Q3/8/8/3K4 b - - 0 1");
    
    auto moves = board.generateMoves();
    EXPECT_EQ(moves, "e6e4|e6e5|e6e7|e8d7|e8e7|e8f7|e8d8|e8f8|");
}

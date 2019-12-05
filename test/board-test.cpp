#include "gtest/gtest.h"
#include "board.h"

TEST(boardTest, emptyBoard)
{
    Board board;
    board.setFENCode("8/8/8/8/8/8/8/8");
    EXPECT_EQ(board.to_string(a8, h8), "--------");
    EXPECT_EQ(board.to_string(a7, h7), "--------");
    EXPECT_EQ(board.to_string(a5, h5), "--------");
    EXPECT_EQ(board.to_string(a5, h5), "--------");
    EXPECT_EQ(board.to_string(a4, h4), "--------");
    EXPECT_EQ(board.to_string(a3, h3), "--------");
    EXPECT_EQ(board.to_string(a2, h2), "--------");
    EXPECT_EQ(board.to_string(a1, h1), "--------");
    EXPECT_EQ(board.getFENCode(), "8/8/8/8/8/8/8/8");
}

TEST(boardTest, fenBoard)
{
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    EXPECT_EQ(board.to_string(a8, h8), "rnbqkbnr");
    EXPECT_EQ(board.to_string(a7, h7), "pppppppp");
    EXPECT_EQ(board.to_string(a6, h6), "--------");
    EXPECT_EQ(board.to_string(a5, h5), "--------");
    EXPECT_EQ(board.to_string(a4, h4), "--------");
    EXPECT_EQ(board.to_string(a3, h3), "--------");
    EXPECT_EQ(board.to_string(a2, h2), "PPPPPPPP");
    EXPECT_EQ(board.to_string(a1, h1), "RNBQKBNR");
    EXPECT_EQ(board.getFENCode(), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
}

TEST(boardTest, getPiecesAtPositions)
{
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    EXPECT_EQ(board.getPiece(a1), 'R');
    EXPECT_EQ(board.getPiece(e1), 'K');
    EXPECT_EQ(board.getPiece(e8), 'k');
    EXPECT_EQ(board.getPiece(e7), 'p');
}

TEST(boardTest, allWhitePieces)
{
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    auto b = board.getWhitePiecesBoard();
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "XXXXXXXX");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "XXXXXXXX");
}

TEST(boardTest, allBlackPieces)
{
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    auto b = board.getBlackPiecesBoard();
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "XXXXXXXX");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "XXXXXXXX");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "--------");
}

TEST(boardTest, possibleMovesKing)
{
    Board board;
    board.setFENCode("7K/8/8/8/8/8/8/8");
    auto b = board.allPossibleMoves(h8);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "------X-");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "------XX");

    board.setFENCode("8/8/8/3K4/8/8/8/8");
    b = board.allPossibleMoves(d5);
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "--XXX---");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--X-X---");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--XXX---");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "--------");
}

TEST(boardTest, rankAttack)
{
    Board board;

    board.setPiece(a1, 'P');
    board.setPiece(b1, 'P');
    board.setPiece(g1, 'P');

    auto pawns = board.getBoard('P');
    // try to calculate row attack when a rock on d1
    auto board1 = board.rankAttack(pawns, d1);
    EXPECT_EQ(board.showOneBitBoard(board1, a1, h1), "-XX-XXX-");

    board.clear();
    board.setPiece(a3, 'K');
    board.setPiece(b3, 'K');
    board.setPiece(c3, 'K');

    auto kings = board.getBoard('K');
    board1 = board.rankAttack(kings, e3);
    EXPECT_EQ(board.showOneBitBoard(board1, a3, h3), "--XX-XXX");
}

TEST(boardTest, diagonalAttack)
{
    Board board;

    board.setPiece(a2, 'P');
    board.setPiece(c4, 'P');
    board.setPiece(f7, 'P');
    auto pawns = board.getBoard('P');
    // try to calculate diagonal attack when a piece on e6
    auto board1 = board.diagonalAttack(pawns, e6);

    EXPECT_EQ(board.showOneBitBoard(board1, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(board1, a7, h7), "-----X--");
    EXPECT_EQ(board.showOneBitBoard(board1, a6, h6), "--------");
    EXPECT_EQ(board.showOneBitBoard(board1, a5, h5), "---X----");
    EXPECT_EQ(board.showOneBitBoard(board1, a4, h4), "--X-----");
    EXPECT_EQ(board.showOneBitBoard(board1, a3, h3), "--------");
    EXPECT_EQ(board.showOneBitBoard(board1, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(board1, a1, h1), "--------");
}

TEST(boardTest, antiDiagonalAttack)
{
    Board board;

    board.setPiece(c8, 'P');
    board.setPiece(g4, 'P');
    board.setPiece(h3, 'P');
    auto pawns = board.getBoard('P');
    // try to calculate antidiagonal attack when a piece on e6
    auto board1 = board.antidiagonalAttack(pawns, e6);

    EXPECT_EQ(board.showOneBitBoard(board1, a8, h8), "--X-----");
    EXPECT_EQ(board.showOneBitBoard(board1, a7, h7), "---X----");
    EXPECT_EQ(board.showOneBitBoard(board1, a6, h6), "--------");
    EXPECT_EQ(board.showOneBitBoard(board1, a5, h5), "-----X--");
    EXPECT_EQ(board.showOneBitBoard(board1, a4, h4), "------X-");
    EXPECT_EQ(board.showOneBitBoard(board1, a3, h3), "--------");
    EXPECT_EQ(board.showOneBitBoard(board1, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(board1, a1, h1), "--------");
}

TEST(boardTest, fileAttack)
{
    Board board;

    board.setPiece(e1, 'P');
    board.setPiece(e3, 'P');
    board.setPiece(e7, 'P');
    auto pawns = board.getBoard('P');
    // try to calculate antidiagonal attack when a piece on e6
    auto board1 = board.fileAttack(pawns, e6);

    EXPECT_EQ(board.showOneBitBoard(board1, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(board1, a7, h7), "----X---");
    EXPECT_EQ(board.showOneBitBoard(board1, a6, h6), "--------");
    EXPECT_EQ(board.showOneBitBoard(board1, a5, h5), "----X---");
    EXPECT_EQ(board.showOneBitBoard(board1, a4, h4), "----X---");
    EXPECT_EQ(board.showOneBitBoard(board1, a3, h3), "----X---");
    EXPECT_EQ(board.showOneBitBoard(board1, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(board1, a1, h1), "--------");
}


TEST(boardTest, possibleMovesKnight)
{
    Board board;
    board.setFENCode("8/8/8/7N/8/8/8/8");
    auto b = board.allPossibleMoves(h5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "------X-");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "-----X--");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "-----X--");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "------X-");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");

    board.setFENCode("8/8/8/6N1/8/8/8/8");
    b = board.allPossibleMoves(g5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "-----X-X");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "----X---");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "----X---");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "-----X-X");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");

    board.setFENCode("8/8/8/5N2/8/8/8/8");
    b = board.allPossibleMoves(f5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "----X-X-");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "---X---X");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "---X---X");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "----X-X-");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");

    board.setFENCode("8/8/8/1N6/8/8/8/8");
    b = board.allPossibleMoves(b5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "X-X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "---X----");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "---X----");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "X-X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");

    board.setFENCode("8/8/8/N7/8/8/8/8");
    b = board.allPossibleMoves(a5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "-X------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "--X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "-X------");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");

    board.setFENCode("8/8/8/8/8/8/8/N7");
    b = board.allPossibleMoves(a1);
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "-X------");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");

    board.setFENCode("7N/8/8/8/8/8/8/8");
    b = board.allPossibleMoves(h8);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "-----X--");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "------X-");
}
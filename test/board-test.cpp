#include "gtest/gtest.h"
#include "board.h"

TEST(boardTest, emptyBoard)
{
    Board board;
    board.setFENCode("8/8/8/8/8/8/8/8");
    EXPECT_EQ(board.to_string(a8, h8), "--------");
    EXPECT_EQ(board.to_string(a7, h7), "----X---");
    EXPECT_EQ(board.to_string(a5, h5), "--------");
    EXPECT_EQ(board.to_string(a5, h5), "--------");
    EXPECT_EQ(board.to_string(a4, h4), "--------");
    EXPECT_EQ(board.to_string(a3, h3), "--------");
    EXPECT_EQ(board.to_string(a2, h2), "--------");
    EXPECT_EQ(board.to_string(a1, h1), "--------");
    EXPECT_EQ(board.getFENCode(), "8/8/8/8/8/8/8/8");
}

TEST(boardTest, fenBoardStartingPosition)
{
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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
    EXPECT_EQ(board.getPieceAt(a1), R);
    EXPECT_EQ(board.getPieceAt(e1), K);
    EXPECT_EQ(board.getPieceAt(e8), k);
    EXPECT_EQ(board.getPieceAt(e7), p);
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
    auto b = board.allPieceMoves(h8);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "------X-");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "------XX");

    board.setFENCode("8/8/8/3K4/8/8/8/8");
    b = board.allPieceMoves(d5);
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "--XXX---");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--X-X---");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--XXX---");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "--------");
}

TEST(boardTest, possibleMovesKingComplex)
{
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/4P3/5P2/PPPP2PP/RNBQKBNR w KQkq - 0 1");
    auto b = board.allPieceMoves(e1);
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "----XX--");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");
}

TEST(boardTest, possibleMovesRookComplex)
{
    Board board;
    board.setFENCode("rnbqkbnr/pppp1ppp/8/3Rp3/2B1P3/5N2/PPPP1PPP/RNBQK3 w Qkq - 2 2");
    auto b = board.allPieceMoves(d5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "---X----");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "---X----");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "XXX-X---");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "---X----");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "---X----");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");
}

TEST(boardTest, rankAttack)
{
    Board board;

    board.setPiece(a1, 'P');
    board.setPiece(b1, 'P');
    board.setPiece(g1, 'P');

    auto pawns = board.getBoard('P');
    // try to calculate row attack when a rook on d1
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
    auto b = board.allPieceMoves(h5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "------X-");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "-----X--");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "-----X--");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "------X-");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");

    board.setFENCode("8/8/8/6N1/8/8/8/8");
    b = board.allPieceMoves(g5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "-----X-X");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "----X---");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "----X---");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "-----X-X");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");

    board.setFENCode("8/8/8/5N2/8/8/8/8");
    b = board.allPieceMoves(f5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "----X-X-");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "---X---X");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "---X---X");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "----X-X-");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");

    board.setFENCode("8/8/8/1N6/8/8/8/8");
    b = board.allPieceMoves(b5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "X-X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "---X----");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "---X----");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "X-X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");

    board.setFENCode("8/8/8/N7/8/8/8/8");
    b = board.allPieceMoves(a5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "-X------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "--X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "-X------");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");

    board.setFENCode("8/8/8/8/8/8/8/N7");
    b = board.allPieceMoves(a1);
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "-X------");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");

    board.setFENCode("7N/8/8/8/8/8/8/8");
    b = board.allPieceMoves(h8);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "-----X--");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "------X-");
}

TEST(boardTest, possibleMovesKnight2)
{
    // complex move from starting position
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    auto b = board.allPieceMoves(b1);
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "X-X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");
}

TEST(boardTest, possibleMovesQueenComplex)
{
    // complex move from starting position
    Board board;
    board.setFENCode("rnb1kbnr/p1pp1ppp/8/1p4p1/3B1q2/8/PPPPPPPP/RNBQK1N1 b Qkq - 2 2");

    auto b = board.allPieceMoves(f4);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "---X-X--");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "----XX--");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "---XX-XX");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "----XXX-");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "---X-X-X");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");
}

TEST(boardTest, possibleMovesWhitePawn)
{
    // move from starting position
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    auto b = board.allPieceMoves(e2);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "----X---");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "----X---");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");
}

TEST(boardTest, possibleMovesBlackPawn)
{
    // move from starting position
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b");

    auto b = board.allPieceMoves(f7);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "-----X--");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "-----X--");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");

    board.setFENCode("rnbqkbnr/pppp1ppp/4p3/5P2/3P4/4P3/PPP3PP/RNBQKBNR b");

    b = board.allPieceMoves(e6);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "----XX--");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");

    // edge of the board
    board.setFENCode("rnbqkbnr/1pppppp1/8/8/p6p/1P4P1/P1PPPP1P/RNBQKBNR b KQkq - 0 1");

    b = board.allPieceMoves(h4);
    b = b | board.allPieceMoves(a4);
    auto str = board.showOneBitBoard(b, a1, h8);

    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "XX----XX");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");

    // edge of the board
    board.setFENCode("rnbqkbnr/p1pppp1p/1p6/P5p1/7P/8/1PPPPPP1/RNBQKBNR w KQkq - 0 1");

    b = board.allPieceMoves(a5);
    b = b | board.allPieceMoves(h4);
    str = board.showOneBitBoard(b, a1, h8);

    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "XX------");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "------XX");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");
}

TEST(boardTest, possibleMovesBishop)
{
    // complex move from starting position
    Board board;
    board.setFENCode("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");

    auto b = board.allPieceMoves(f1);
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "X-------");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "-X------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--X-----");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "---X----");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "----X---");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");

    // more complex case with a white bishop
    board.setFENCode("rnbqkbnr/p1ppp1pp/1p3p2/8/3BP3/2P5/PPP2PPP/RNBQK1NR w KQkq - 0 1");
    b = board.allPieceMoves(d4);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "-X---X--");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--X-X---");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "----X---");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");

    // complex case with a black bishop
    board.setFENCode("rnbqk1nr/p1pppppp/1p6/4b3/4P3/2P1B3/PPP2PPP/RNBQK1NR b KQkq - 0 1");
    b = board.allPieceMoves(e5);
    EXPECT_EQ(board.showOneBitBoard(b, a8, h8), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a7, h7), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a6, h6), "---X-X--");
    EXPECT_EQ(board.showOneBitBoard(b, a5, h5), "--------");
    EXPECT_EQ(board.showOneBitBoard(b, a4, h4), "---X-X--");
    EXPECT_EQ(board.showOneBitBoard(b, a3, h3), "--X---X-");
    EXPECT_EQ(board.showOneBitBoard(b, a2, h2), "-------X");
    EXPECT_EQ(board.showOneBitBoard(b, a1, h1), "--------");
}

TEST(boardTest, castlingFEN)
{
    Board board;
    board.setFENCode("rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1");

    EXPECT_EQ(board.castling & CASTLING_BLACK_KINGSIDE, CASTLING_BLACK_KINGSIDE);
    EXPECT_EQ(board.castling & CASTLING_BLACK_QUEENSIDE, CASTLING_BLACK_QUEENSIDE);
    EXPECT_EQ(board.castling & CASTLING_WHITE_KINGSIDE, CASTLING_WHITE_KINGSIDE);
    EXPECT_EQ(board.castling & CASTLING_WHITE_QUEENSIDE, CASTLING_WHITE_QUEENSIDE);   
}

TEST(boardTest, entpassant)
{
    Board board;
    board.setFENCode("r1bqkbnr/ppppp1p1/2n2p2/7p/3PPP2/8/PPP3PP/RNBQKBNR w KQkq h6 0 4");

    EXPECT_EQ(board.enpassant, h6);
}
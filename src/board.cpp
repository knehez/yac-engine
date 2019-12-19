#include "board.h"

Board::Board()
{
    clear();

    for (int x = a1; x < NUMBER_OF_SQUARES; x++)
    {
        for (int f = 0; f < 8; f++)
        {
            RANK_ATTACK[x * 8 + f] = generate_rank_attack(x * 2, f);
        }
        // http://cinnamonchess.altervista.org/bitboard_calculator/Calc.html
        int file = x & 7;
        int rank = x >> 3;
        MASK[x].file = FILES[file];

        MASK[x].diagonal = 0;
        // file diagonal bits UP
        for (int i = x; i < NUMBER_OF_SQUARES; i += 9)
        {
            MASK[x].diagonal |= 1ULL << i;
            file++;
            rank++;

            if (file > 7 || rank > 7)
            {
                break;
            }
        }

        // file diagonal bits down
        file = x & 7;
        rank = x >> 3;

        for (int i = x; i >= a1; i -= 9)
        {
            MASK[x].diagonal |= 1ULL << i;
            file--;
            rank--;

            if (file < 0 || rank < 0)
            {
                break;
            }
        }

        MASK[x].antidiagonal = 0;

        // file antidiagonal bits up
        file = x & 7;
        rank = x >> 3;
        for (int i = x; i <= h8; i += 7)
        {
            MASK[x].antidiagonal |= 1ULL << i;
            file--;
            rank++;

            if (file < 0 || rank > 7)
            {
                break;
            }
        }

        file = x & 7;
        rank = x >> 3;
        // file antidiagonal bits down
        for (int i = x; i >= a1; i -= 7)
        {
            MASK[x].antidiagonal |= 1ULL << i;
            file++;
            rank--;

            if (file > 7 || rank < 0)
            {
                break;
            }
        }
    }

    color = WHITE;
}

Position Board::nextSquare(uint64_t *board)
{
    if (*board == 0)
    {
        return NUMBER_OF_SQUARES;
    }
    auto index = bitScan64(*board);
    *board &= *board - 1;
    return (Position)index;
}

void Board::clear()
{
    for (int i = 0; i < NUMBER_OF_PIECES; i++)
    {
        m_boards[i] = 0;
    }
}

uint64_t Board::getWhitePiecesBoard()
{
    return m_boards[P] | m_boards[R] | m_boards[N] | m_boards[B] | m_boards[Q] | m_boards[K];
}

uint64_t Board::getBlackPiecesBoard()
{
    return m_boards[p] | m_boards[r] | m_boards[n] | m_boards[b] | m_boards[q] | m_boards[k];
}

void Board::setPiece(int pos, char piece)
{
    for (int i = 0; i < NUMBER_OF_PIECES; i++)
    {
        if (piece == piecesChars[i])
        {
            m_boards[i] |= 1LLU << pos;
            break;
        }
    }
}

uint64_t Board::rankAttack(uint64_t occupancy, Position pos)
{
    int f = pos & 7;
    int r = pos & 56;
    uint64_t o = (occupancy >> r) & 126;
    return uint64_t(RANK_ATTACK[o * 4 + f]) << r;
}

/* Generate attack using the hyperbola quintessence approach */
uint64_t Board::attack(uint64_t board, Position pos, uint64_t movesMask)
{
    uint64_t o = board & movesMask;
    uint64_t r = bit_bswap(o);

    return ((o - (1ULL << pos)) ^ bit_bswap(r - (1ULL << (pos ^ 56)))) & movesMask;
}

uint64_t Board::diagonalAttack(uint64_t pieces, Position x)
{
    return attack(pieces, x, MASK[x].diagonal);
}

uint64_t Board::antidiagonalAttack(uint64_t pieces, Position x)
{
    return attack(pieces, x, MASK[x].antidiagonal);
}

uint64_t Board::fileAttack(uint64_t pieces, Position x)
{
    return attack(pieces, x, MASK[x].file);
}

uint64_t Board::getBoard(char piece)
{
    for (int i = 0; i < NUMBER_OF_PIECES; i++)
    {
        if (piece == piecesChars[i])
        {
            return m_boards[i];
        }
    }
    return 0LLU;
}

std::string Board::getFENCode()
{
    std::string fen;
    int rowNumber = 7;
    int boardPos = 8 * rowNumber;
    auto emptyCounter{0};
    for (int i = h8; i >= a1; i--)
    {
        auto found{false};
        for (int j = 0; j < NUMBER_OF_PIECES; j++)
        {
            if (m_boards[j] & (1LLU << boardPos))
            {
                if (emptyCounter != 0)
                {
                    fen += std::to_string(emptyCounter);
                    emptyCounter = 0;
                }
                fen += piecesChars[j];
                found = true;
                break;
            }
        }
        if (found == false)
        {
            emptyCounter++;
        }
        if (i % 8 == 0)
        {
            rowNumber--;
            boardPos = 8 * rowNumber;
            if (emptyCounter)
            {
                fen += std::to_string(emptyCounter);
                emptyCounter = 0;
            }
            if (i != 0) // do not write out last '/'
            {
                fen += '/';
            }
            continue; // do not increase position
        }
        boardPos++;
    }
    return fen;
}

void Board::setFENCode(char *fenCode)
{
    clear();
    int rowNumber = 7;
    int boardPos = 8 * rowNumber;

    do
    {
        int emptyPositions = atoi(fenCode);
        if (emptyPositions != 0)
        {
            for (int i = 0; i < emptyPositions; i++)
            {
                for (int j = 0; j < NUMBER_OF_PIECES; j++)
                {
                    // set empty position of all the boards
                    m_boards[j] &= ~(1LLU << boardPos);
                }
                boardPos++;
            }
            continue;
        }
        for (int i = 0; i < NUMBER_OF_PIECES; i++)
        {
            if (*fenCode == piecesChars[i])
            {
                m_boards[i] |= (1LLU << boardPos);
                break;
            }
        }
        if (*fenCode == '/')
        {
            rowNumber--;
            boardPos = 8 * rowNumber;
            continue;
        }

        if (*fenCode == ' ')
        {
            fenCode++;
            break;
        }
        boardPos++;
    } while (*(++fenCode));

    // w or b after the first space
    *(fenCode++) == 'w' ? color = WHITE : color = BLACK;

    // step out next space
    fenCode++;

    castling = 0;

    do
    {
        switch (*fenCode)
        {
        case 'K':
            castling |= CASTLING_WHITE_KINGSIDE;
            break;
        case 'Q':
            castling |= CASTLING_WHITE_QUEENSIDE;
            break;
        case 'k':
            castling |= CASTLING_BLACK_KINGSIDE;
            break;
        case 'q':
            castling |= CASTLING_BLACK_QUEENSIDE;
            break;
        case '-':
            break;
        }
        // exit when space or '-'
        if (*fenCode == ' ')
        {
            fenCode++;
            break;
        }
    } while (*(fenCode++));

    // processing enpassant move
    if (*fenCode == '-')
    {
        fenCode++;
    }
    else
    {
        int file = *(fenCode++) - 'a';
        int rank = *(fenCode++) - '1';
        int pos = rank * 8 + file;
        enpassant = (Position)pos;
    }
}

Piece Board::getPieceAt(Position pos)
{
    for (int j = 0; j < NUMBER_OF_PIECES; j++)
    {
        if (m_boards[j] & (1LLU << pos))
        {
            return (Piece)j;
        }
    }
    return NUMBER_OF_PIECES;
}

void Board::move(Move move)
{
    auto &movingBoard = m_boards[move.piece];

    if (move.captured != NUMBER_OF_PIECES)
    {
        auto &capturedBoard = m_boards[move.captured];
        // remove captured piece
        capturedBoard &= ~(1LLU << move.end);
    }

    // move piece
    movingBoard &= ~(1LLU << move.start);

    if (move.promotion == NUMBER_OF_PIECES)
    {
        // normal move
        movingBoard |= 1LLU << move.end;
    }
    else
    {
        auto &promotionBoard = m_boards[move.promotion];
        promotionBoard |= 1LLU << move.end;
    }

    color = oppositeColor(color);
}

void Board::undoMove(Move move)
{
    auto &movingBoard = m_boards[move.piece];
    if (move.captured != NUMBER_OF_PIECES)
    {
        auto &capturedBoard = m_boards[move.captured];
        // put back captured piece
        capturedBoard |= 1LLU << move.end;
    }
    // if there is a promotion then clear the promoted piece
    if (move.promotion != NUMBER_OF_PIECES)
    {
        auto &promotionBoard = m_boards[move.promotion];
        promotionBoard &= ~(1LLU << move.end);
    }
    // move back moving piece
    movingBoard &= ~(1LLU << move.end);
    movingBoard |= (1LLU << move.start);

    color = oppositeColor(color);
}

std::string Board::generateMoves()
{
    std::string moves;

    uint64_t board;
    color == WHITE ? board = getWhitePiecesBoard() : board = getBlackPiecesBoard();

    while (board)
    {
        auto actualPiecePos = nextSquare(&board);

        auto tmpBoard = allPieceMoves(actualPiecePos);

        auto nextPromotion = (int)R;

        if (tmpBoard != 0)
        {
            int endPos;
            while ((endPos = nextSquare(&tmpBoard)) < NUMBER_OF_SQUARES)
            {
                Move tryMove;
                tryMove.start = (Position)actualPiecePos;
                tryMove.end = (Position)endPos;
                tryMove.piece = getPieceAt((Position)actualPiecePos);
                tryMove.captured = getPieceAt((Position)endPos);
                auto piecerank = rank(tryMove.end);
                // white pawn on the rank H - black pawn on the rank A?
                if ((piecerank == 7 && tryMove.piece == P) | (piecerank == 0 && tryMove.piece == p))
                {
                    tryMove.promotion = (Piece)(nextPromotion + color);
                    nextPromotion += 2;
                }

                move(tryMove);
                auto oppositeKingPos = color == BLACK ? getPiecePos(K) : getPiecePos(k);

                // is in check?
                if (!isSqareAttacked(oppositeKingPos, &m_boards[0], color))
                {
                    // valid move
                    moves += algebraicFile(actualPiecePos);
                    moves += algebraicRank(actualPiecePos);
                    moves += algebraicFile(endPos);
                    moves += algebraicRank(endPos);
                    // is there a promotion?
                    if (tryMove.promotion != NUMBER_OF_PIECES)
                    {
                        moves += "(";
                        moves += piecesChars[tryMove.promotion];
                        moves += ")";
                        if (nextPromotion > Q)
                        {
                            // no more promotion, do not put back pawn
                            // set nextPromotion to 0 (Rook).
                            nextPromotion = (int)R;
                        }
                        else
                        {
                            // put back Pawn for the next promotion
                            tmpBoard |= (1LLU << tryMove.end);
                        }
                    }
                    moves += '|';
                }
                undoMove(tryMove);
            }
        }
    }
    // casting is possible?
    color == WHITE ? board = getWhitePiecesBoard() : board = getBlackPiecesBoard();
    if (castling == 0)
    {
        return moves;
    }

    if (color == WHITE && !isSqareAttacked(getPiecePos(K), &m_boards[0], oppositeColor(color)))
    {
        if (castling & CASTLING_WHITE_KINGSIDE)
        {
            // king on e1, rook on h1 and f1 and g1 empty
            // king on f1, g1 not in check
            if (getPiecePos(K) == e1 && getPieceAt(h1) == R &&
                getPieceAt(f1) == NUMBER_OF_PIECES && getPieceAt(g1) == NUMBER_OF_PIECES &&
                !isSqareAttacked(f1, &m_boards[0], oppositeColor(color)) &&
                !isSqareAttacked(g1, &m_boards[0], oppositeColor(color)))
            {
                moves += "e1g1|";
                castling &= ~(CASTLING_WHITE_KINGSIDE | CASTLING_WHITE_QUEENSIDE);
            }
        }
        if (castling & CASTLING_WHITE_QUEENSIDE)
        {
            // king on e1, rook on a1, b1 c1 d1 empty
            // king on e1, d1, c1 not in check
            if (getPiecePos(K) == e1 && getPieceAt(a1) == R &&
                getPieceAt(b1) == NUMBER_OF_PIECES && getPieceAt(c1) == NUMBER_OF_PIECES && getPieceAt(d1) == NUMBER_OF_PIECES &&
                !isSqareAttacked(d1, &m_boards[0], oppositeColor(color)) &&
                !isSqareAttacked(c1, &m_boards[0], oppositeColor(color)))
            {
                moves += "e1c1|";
                castling &= ~(CASTLING_WHITE_KINGSIDE | CASTLING_WHITE_QUEENSIDE);
            }
        }
    }
    if (color == BLACK && !isSqareAttacked(getPiecePos(k), &m_boards[0], oppositeColor(color)))
    {
        if (castling & CASTLING_BLACK_KINGSIDE)
        {
            // king on e8, rook on h8 and f8 and g8 are empty
            // king on f8, g8 not in check
            if (getPiecePos(k) == e8 && getPieceAt(h8) == r &&
                getPieceAt(f8) == NUMBER_OF_PIECES && getPieceAt(g8) == NUMBER_OF_PIECES &&
                !isSqareAttacked(f8, &m_boards[0], oppositeColor(color)) &&
                !isSqareAttacked(g8, &m_boards[0], oppositeColor(color)))
            {
                moves += "e8g8|";
                castling &= ~(CASTLING_BLACK_KINGSIDE | CASTLING_BLACK_QUEENSIDE);
            }
        }
        if (castling & CASTLING_BLACK_QUEENSIDE)
        {
            // king on e8, rook on a8, b8 c8 d8 are empty
            // king on e1, d1, c1 not in check
            if (getPiecePos(k) == e8 && getPieceAt(a8) == r &&
                getPieceAt(b8) == NUMBER_OF_PIECES && getPieceAt(c8) == NUMBER_OF_PIECES && getPieceAt(d8) == NUMBER_OF_PIECES &&
                !isSqareAttacked(d8, &m_boards[0], oppositeColor(color)) &&
                !isSqareAttacked(c8, &m_boards[0], oppositeColor(color)))
            {
                moves += "e8c8|";
                castling &= ~(CASTLING_BLACK_KINGSIDE | CASTLING_BLACK_QUEENSIDE);
            }
        }
    }
    return moves;
}

bool Board::isSqareAttacked(Position square, uint64_t *board, Color oppositeColor)
{
    auto oppositeKnights = board[N + oppositeColor];
    auto oppositePawns = board[P + oppositeColor];
    auto oppositeRookQueen = board[Q + oppositeColor];
    auto oppositeBishopQueen = oppositeRookQueen;
    oppositeRookQueen |= board[R + oppositeColor];
    oppositeBishopQueen |= board[B + oppositeColor];

    return (knightMoves(square) & oppositeKnights) |
           ((oppositeColor == WHITE ? pawnBlackHitMoves(square) : pawnWhiteHitMoves(square)) & oppositePawns) |
           (bishopMoves(square) & oppositeBishopQueen) |
           (rookMoves(square) & oppositeRookQueen);
}

// good for piceces where there is only one
Position Board::getPiecePos(Piece piece)
{
    // select board
    auto board = m_boards[piece];
    // determinate the first bit
    auto index = bitScan64(board);
    return (Position)index;
}

uint64_t Board::allPieceMoves(Position pos)
{
    Piece piece = getPieceAt((Position)pos);

    auto notOwnPieces = (color == WHITE ? ~getWhitePiecesBoard() : ~getBlackPiecesBoard());
    uint64_t oppositePieces;

    switch (piece)
    {
    case N:
    case n:
        return knightMoves(pos) & notOwnPieces;
    case B:
    case b:
        return bishopMoves(pos) & notOwnPieces;
    case K:
    case k:
        return kingMoves(pos) & notOwnPieces;
    case P:
        oppositePieces = color == BLACK ? getWhitePiecesBoard() : getBlackPiecesBoard();
        return pawnWhiteMoves(pos) | (pawnWhiteHitMoves(pos) & oppositePieces);
    case p:
        oppositePieces = color == BLACK ? getWhitePiecesBoard() : getBlackPiecesBoard();
        return pawnBlackMoves(pos) | (pawnBlackHitMoves(pos) & oppositePieces);
    case R:
    case r:
        return rookMoves(pos) & notOwnPieces;
    case Q:
    case q:
        return (rookMoves(pos) | bishopMoves(pos)) & notOwnPieces;
    default:
        return 0;
    }
}

uint64_t Board::rookMoves(Position pos)
{
    auto allPiecesAsBitmap = getWhitePiecesBoard() | getBlackPiecesBoard();
    auto removeRookAtPosition = ~(1LLU << (Position)pos);

    uint64_t rank = rankAttack(allPiecesAsBitmap & removeRookAtPosition, (Position)pos);
    uint64_t file = fileAttack(allPiecesAsBitmap & removeRookAtPosition, (Position)pos);
    return rank | file;
}

uint64_t Board::bishopMoves(Position pos)
{
    auto allPiecesAsBitmap = getWhitePiecesBoard() | getBlackPiecesBoard();
    auto notBishopAtPosition = ~(1LLU << (Position)pos);

    uint64_t diagonal = diagonalAttack((allPiecesAsBitmap & notBishopAtPosition), (Position)pos);

    uint64_t antiDiagonal = antidiagonalAttack(allPiecesAsBitmap & notBishopAtPosition, (Position)pos);

    return diagonal | antiDiagonal;
}

uint64_t Board::knightMoves(Position pos)
{
    uint64_t tmp = 1LLU << pos;
    uint64_t ret = KNIGHT_NORTH_EAST(tmp) & notHFile;
    ret |= KNIGHT_NORTH_WEST(tmp) & notAFile;
    ret |= KNIGHT_EAST_NORTH(tmp) & notGHFile;
    ret |= KNIGHT_EAST_SOUTH(tmp) & notABFile;
    ret |= KNIGHT_WEST_NORTH(tmp) & notABFile;
    ret |= KNIGHT_WEST_SOUTH(tmp) & notGHFile;
    ret |= KNIGHT_SOUTH_EAST(tmp) & notHFile;
    ret |= KNIGHT_SOUTH_WEST(tmp) & notAFile;
    return ret;
}

uint64_t Board::pawnBlackMoves(Position pos)
{
    uint64_t tmp = 1LLU << pos;
    // move south if there is not any pieces in front of the pawn
    uint64_t ret = SOUTH(tmp) & ~(getWhitePiecesBoard() | getBlackPiecesBoard());
    // pawn can move 2 squares if it is on the second row () (0xff00000000)
    ret |= SOUTH_TWO(tmp) & 0xff00000000; // row 'f'
    return ret;
}

uint64_t Board::pawnBlackHitMoves(Position pos)
{
    uint64_t tmp = 1LLU << pos;
    // move south if there is not any pieces in front of the pawn
    uint64_t ret = ((SOUTH_EAST(tmp) & notHFile) | (SOUTH_WEST(tmp) & notAFile));
    return ret;
}

uint64_t Board::pawnWhiteHitMoves(Position pos)
{
    uint64_t tmp = 1LLU << pos;
    // move south if there is not any pieces in front of the pawn
    uint64_t ret = ((NORTH_EAST(tmp) & notAFile) | (NORTH_WEST(tmp) & notHFile));
    return ret;
}

uint64_t Board::pawnWhiteMoves(Position pos)
{
    uint64_t tmp = 1LLU << pos;
    // move north if there is not any pieces in front of the pawn
    uint64_t ret = NORTH(tmp) & ~(getWhitePiecesBoard() | getBlackPiecesBoard());
    // pawn can move 2 squares if it is on the second row (0xff000000)
    ret |= NORTH_TWO(tmp) & 0xff000000; // row 'e'
    return ret;
}

uint64_t Board::kingMoves(Position pos)
{
    uint64_t tmp = 1LLU << pos;
    uint64_t ret = NORTH(tmp);
    ret |= SOUTH(tmp);
    ret |= EAST(tmp) & notHFile;
    ret |= WEST(tmp) & notAFile;
    ret |= NORTH_EAST(tmp) & notHFile;
    ret |= NORTH_WEST(tmp) & notAFile;
    ret |= SOUTH_EAST(tmp) & notHFile;
    ret |= SOUTH_WEST(tmp) & notAFile;
    return ret;
}

std::string Board::showOneBitBoard(uint64_t board, int startPos, int endPos)
{
    std::string out;
    int rowNumber = endPos / 8;
    int boardPos = 8 * rowNumber;

    for (int i = endPos; i >= startPos; i--)
    {

        if (board & (1LLU << boardPos))
        {
            out += 'X';
        }
        else
        {
            out += '-';
        }
        if (i % 8 == 0)
        {
            rowNumber--;
            boardPos = 8 * rowNumber;
            out += "\n";
        }
        else
        {
            boardPos++;
        }
    }
    trimWhiteSpaces(out);
    return out;
}

std::string Board::to_string(int startPos, int endPos)
{
    std::string out;
    int rowNumber = endPos / 8;
    int boardPos = 8 * rowNumber;

    for (int i = endPos; i >= startPos; i--)
    {
        bool found = false;
        for (int j = 0; j < NUMBER_OF_PIECES; j++)
        {
            if (m_boards[j] & (1LLU << boardPos))
            {
                out += piecesChars[j];
                // do not find an other one if we already found one
                found = true;
                break;
            }
        }

        if (found == false)
        {
            out += '-';
        }
        if (i % 8 == 0)
        {
            rowNumber--;
            boardPos = 8 * rowNumber;
            out += '\n';
        }
        else
        {
            boardPos++;
        }
    }
    trimWhiteSpaces(out);
    return out;
}

void Board::trimWhiteSpaces(std::string &str)
{
    if (str.empty())
        return;

    // Trim spaces from right side
    size_t len = str.size();

    while (str.rfind("\n") == --len)
    {
        str.erase(len, len + 1);
    }
}

int Board::generate_rank_attack(int o, int f)
{
    int x, y;
    int b;

    y = 0;
    for (x = f - 1; x >= 0; --x)
    {
        b = 1 << x;
        y |= b;
        if ((o & b) == b)
        {
            break;
        }
    }
    for (x = f + 1; x < 8; ++x)
    {
        b = 1 << x;
        y |= b;
        if ((o & b) == b)
        {
            break;
        }
    }
    return y;
}

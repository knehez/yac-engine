#include "chessboard.h"
#include <iostream>
#include <fstream>

ChessBoard::ChessBoard()
{
    // debugging rsult file
    fenFile.open("fenResult.txt");
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
    initialState();
}

void ChessBoard::initialState()
{
    state->color = WHITE;
}

Position ChessBoard::nextSquare(uint64_t *board)
{
    if (*board == 0)
    {
        return NUMBER_OF_SQUARES;
    }
    auto index = bitScan64(*board);
    *board &= *board - 1;
    return (Position)index;
}

void ChessBoard::clear()
{
    for (int i = 0; i < NUMBER_OF_PIECES; i++)
    {
        state->boards[i] = 0;
    }
}

uint64_t ChessBoard::getWhitePiecesBoard()
{
    return state->boards[P] | state->boards[R] | state->boards[N] | state->boards[B] | state->boards[Q] | state->boards[K];
}

uint64_t ChessBoard::getBlackPiecesBoard()
{
    return state->boards[p] | state->boards[r] | state->boards[n] | state->boards[b] | state->boards[q] | state->boards[k];
}

void ChessBoard::setPiece(int pos, char piece)
{
    for (int i = 0; i < NUMBER_OF_PIECES; i++)
    {
        if (piece == piecesChars[i])
        {
            state->boards[i] |= 1LLU << pos;
            break;
        }
    }
}

uint64_t ChessBoard::rankAttack(uint64_t occupancy, Position pos)
{
    int f = pos & 7;
    int r = pos & 56;
    uint64_t o = (occupancy >> r) & 126;
    return uint64_t(RANK_ATTACK[o * 4 + f]) << r;
}

/* Generate attack using the hyperbola quintessence approach */
uint64_t ChessBoard::attack(uint64_t board, Position pos, uint64_t movesMask)
{
    uint64_t o = board & movesMask;
    uint64_t r = bit_bswap(o);

    return ((o - (1ULL << pos)) ^ bit_bswap(r - (1ULL << (pos ^ 56)))) & movesMask;
}

uint64_t ChessBoard::diagonalAttack(uint64_t pieces, Position x)
{
    return attack(pieces, x, MASK[x].diagonal);
}

uint64_t ChessBoard::antidiagonalAttack(uint64_t pieces, Position x)
{
    return attack(pieces, x, MASK[x].antidiagonal);
}

uint64_t ChessBoard::fileAttack(uint64_t pieces, Position x)
{
    return attack(pieces, x, MASK[x].file);
}

uint64_t ChessBoard::getBoard(char piece)
{
    for (int i = 0; i < NUMBER_OF_PIECES; i++)
    {
        if (piece == piecesChars[i])
        {
            return state->boards[i];
        }
    }
    return 0LLU;
}

std::string ChessBoard::getFENCode()
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
            if (state->boards[j] & (1LLU << boardPos))
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

    fen += ' ';
    state->color == WHITE ? fen += 'w' : fen += 'b';

    fen += ' ';
    if (state->castling != 0)
    {
        if (state->castling & CASTLING_WHITE_KINGSIDE)
        {
            fen += 'K';
        }
        if (state->castling & CASTLING_WHITE_QUEENSIDE)
        {
            fen += 'Q';
        }
        if (state->castling & CASTLING_BLACK_KINGSIDE)
        {
            fen += 'k';
        }
        if (state->castling & CASTLING_BLACK_QUEENSIDE)
        {
            fen += 'q';
        }
    }
    else
    {
        fen += '-';
    }

    fen += ' ';

    if (state->enpassant != NUMBER_OF_SQUARES)
    {
        fen += to_string(state->enpassant);
    }
    else
    {
        fen += '-';
    }
    fen += " 0 1";
    return fen;
}

void ChessBoard::setFENCode(const char *fenCode)
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
                    state->boards[j] &= ~(1LLU << boardPos);
                }
                boardPos++;
            }
            continue;
        }
        for (int i = 0; i < NUMBER_OF_PIECES; i++)
        {
            if (*fenCode == piecesChars[i])
            {
                state->boards[i] |= (1LLU << boardPos);
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
    *(fenCode++) == 'w' ? state->color = WHITE : state->color = BLACK;

    // step out next space
    fenCode++;

    state->castling = 0;

    do
    {
        switch (*fenCode)
        {
        case 'K':
            state->castling |= CASTLING_WHITE_KINGSIDE;
            break;
        case 'Q':
            state->castling |= CASTLING_WHITE_QUEENSIDE;
            break;
        case 'k':
            state->castling |= CASTLING_BLACK_KINGSIDE;
            break;
        case 'q':
            state->castling |= CASTLING_BLACK_QUEENSIDE;
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
        if (!(*fenCode >= 'a' && *fenCode <= 'h'))
        { // invalid FEN code
            return;
        }
        int file = *(fenCode++) - 'a';
        if (!(*fenCode >= '1' && *fenCode <= '8'))
        { // invalid FEN code
            return;
        }
        int rank = *(fenCode++) - '1';
        int pos = rank * 8 + file;
        state->enpassant = (Position)pos;
    }
}

Piece ChessBoard::getPieceAt(Position pos)
{
    for (int j = 0; j < NUMBER_OF_PIECES; j++)
    {
        if (state->boards[j] & (1LLU << pos))
        {
            return (Piece)j;
        }
    }
    return NUMBER_OF_PIECES;
}

bool ChessBoard::validateMove(Move *userMove)
{
    Moves moves;
    moves = generateMoves(moves);

    if (getPieceAt(userMove->end) != NUMBER_OF_PIECES)
    {
        userMove->captured = getPieceAt(userMove->end);
    }

    for (int i = 0; i < moves.length; i++)
    {
        Move m = moves.move[i];
        if (m.start == userMove->start && m.end == userMove->end)
        {
            return true;
        }
    }
    return false;
}

void ChessBoard::move(Move move)
{
    // init a new state
    BoardState *nextState = state + 1;

    memcpy(nextState->boards, state->boards, sizeof(uint64_t) * NUMBER_OF_PIECES);

    nextState->color = state->color;
    nextState->castling = state->castling;
    nextState->enpassant = NUMBER_OF_SQUARES;
    state++;

    auto movingPiece = getPieceAt(move.start);
    auto &movingBoard = state->boards[movingPiece];

    // remove castling posibility if king or rook moved
    if (state->castling & CASTLING_WHITE_KINGSIDE)
    {
        if (movingPiece == R && move.start == h1)
        {
            state->castling &= ~CASTLING_WHITE_KINGSIDE;
        }
        else if (movingPiece == K && abs(move.start - move.end) != 2)
        {
            state->castling &= ~CASTLING_WHITE_KINGSIDE;
        }
    }
    if (state->castling & CASTLING_WHITE_QUEENSIDE)
    {
        if (movingPiece == R && move.start == a1)
        {
            state->castling &= ~CASTLING_WHITE_QUEENSIDE;
        }
        else if (movingPiece == K && abs(move.start - move.end) != 2)
        {
            state->castling &= ~CASTLING_WHITE_QUEENSIDE;
        }
    }
    if (state->castling & CASTLING_BLACK_KINGSIDE)
    {
        if (movingPiece == r && move.start == h8)
        {
            state->castling &= ~CASTLING_BLACK_KINGSIDE;
        }
        else if (movingPiece == k && abs(move.start - move.end) != 2)
        {
            state->castling &= ~CASTLING_BLACK_KINGSIDE;
        }
    }
    if (state->castling & CASTLING_BLACK_QUEENSIDE)
    {
        if (movingPiece == r && move.start == a8)
        {
            state->castling &= ~CASTLING_BLACK_QUEENSIDE;
        }
        else if (movingPiece == k && abs(move.start - move.end) != 2)
        {
            state->castling &= ~CASTLING_BLACK_QUEENSIDE;
        }
    }
    // set enpassant flag if a pawn moves 2 squares
    if (movingPiece == P && move.end - move.start == 16)
    {
        state->enpassant = (Position)(move.start + 8);
    }
    else if (movingPiece == p && move.start - move.end == 16)
    {
        state->enpassant = (Position)(move.start - 8);
    }
    else
    {
        state->enpassant = NUMBER_OF_SQUARES;
    }

    if (move.captured != NUMBER_OF_PIECES)
    {
        auto &capturedBoard = state->boards[move.captured];
        if (move.enpassant != NUMBER_OF_SQUARES)
        {
            // remove enpassant piece
            auto hitPosition = (state->color == BLACK) ? move.enpassant + 8 : move.enpassant - 8;
            capturedBoard &= ~(1LLU << (Position)hitPosition);
            // switch off enpassant
            state->enpassant = NUMBER_OF_SQUARES;
        }
        else
        { // normal hit
            // remove captured piece
            capturedBoard &= ~(1LLU << move.end);
        }
    }
    // CASTLING
    //
    // - detect castling (4 types)
    // - move rock in castling
    //
    if (state->castling != 0)
    {
        if (movingPiece == R && move.start == h1)
        {
            state->castling &= ~(CASTLING_WHITE_KINGSIDE);
        }
        if (movingPiece == R && move.start == a1)
        {
            state->castling &= ~(CASTLING_WHITE_QUEENSIDE);
        }
        if (movingPiece == r && move.start == h8)
        {
            state->castling &= ~(CASTLING_BLACK_KINGSIDE);
        }
        if (movingPiece == r && move.start == a8)
        {
            state->castling &= ~(CASTLING_BLACK_QUEENSIDE);
        }
        if (move.start == e1 && move.end == g1 && movingPiece == K)
        {
            state->boards[R] &= ~(1LLU << h1);
            state->boards[R] |= (1LLU << f1);
            state->castling &= ~(CASTLING_WHITE_KINGSIDE | CASTLING_WHITE_QUEENSIDE);
        }
        else if (move.start == e1 && move.end == c1 && movingPiece == K)
        {
            state->boards[R] &= ~(1LLU << a1);
            state->boards[R] |= (1LLU << d1);
            state->castling &= ~(CASTLING_WHITE_KINGSIDE | CASTLING_WHITE_QUEENSIDE);
        }
        else if (move.start == e8 && move.end == g8 && movingPiece == k)
        {
            state->boards[r] &= ~(1LLU << h8);
            state->boards[r] |= (1LLU << f8);
            state->castling &= ~(CASTLING_BLACK_KINGSIDE | CASTLING_BLACK_QUEENSIDE);
        }
        else if (move.start == e8 && move.end == c8 && movingPiece == k)
        {
            state->boards[r] &= ~(1LLU << a8);
            state->boards[r] |= (1LLU << d8);
            state->castling &= ~(CASTLING_BLACK_KINGSIDE | CASTLING_BLACK_QUEENSIDE);
        }
    }
    // move a general piece
    movingBoard ^= (1LLU << move.start);

    if (move.promotion == NUMBER_OF_PIECES)
    {
        // normal move
        movingBoard |= 1LLU << move.end;
    }
    else
    {
        auto &promotionBoard = state->boards[move.promotion];
        promotionBoard |= 1LLU << move.end;
    }

    state->color = oppositeColor(state->color);
}

void ChessBoard::undoMove(Move move)
{
    state--;
}

bool ChessBoard::isMate()
{
    Moves m;
    auto moves = generateMoves(m);
    auto kingPosition = (state->color == WHITE) ? getPiecePos(K) : getPiecePos(k);
    auto oppositeColor = (state->color == WHITE) ? BLACK : WHITE;
    // mate: no moves and king is in check
    if (moves.length == 0 && isSqareAttacked(kingPosition, &state->boards[0], oppositeColor) == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ChessBoard::isStaleMate()
{
    Moves m;
    auto moves = generateMoves(m);
    auto kingPosition = (state->color == WHITE) ? getPiecePos(K) : getPiecePos(k);
    // stale mate: no moves and king is not is check
    if (moves.length == 0 && isSqareAttacked(kingPosition, &state->boards[0], state->color) == false)
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::string too_string(Moves moves)
{
    std::string strMove;
    for (int i = 0; i < moves.length; i++)
    {
        strMove += algebraicFile(moves.move[i].start);
        strMove += algebraicRank(moves.move[i].start);
        strMove += algebraicFile(moves.move[i].end);
        strMove += algebraicRank(moves.move[i].end);
        if (moves.move[i].promotion != NUMBER_OF_PIECES)
        {
            strMove += "(";
            strMove += piecesChars[moves.move[i].promotion];
            strMove += ")";
        }
        strMove += "|";
    }
    return strMove;
}

std::string ChessBoard::to_string(Position pos)
{
    std::string strPos;
    strPos += algebraicFile(pos);
    strPos += algebraicRank(pos);
    return strPos;
}

std::string ChessBoard::to_string(Move move)
{
    std::string strMove;
    strMove += algebraicFile(move.start);
    strMove += algebraicRank(move.start);
    strMove += algebraicFile(move.end);
    strMove += algebraicRank(move.end);
    if (move.promotion != NUMBER_OF_PIECES)
    {
        strMove += tolower(piecesChars[move.promotion]);
    }
    return strMove;
}

Color ChessBoard::getColor()
{
    return state->color;
}

Moves ChessBoard::generateMoves(Moves moves)
{
    auto startMove = &moves.move[0];
    auto currentMove = startMove;
    uint64_t board;

    state->color == WHITE ? board = getWhitePiecesBoard() : board = getBlackPiecesBoard();

    uint64_t tempBoard[NUMBER_OF_PIECES];

    while (board)
    {
        auto actualPiecePos = nextSquare(&board);
        if(actualPiecePos < 0 || actualPiecePos > NUMBER_OF_SQUARES)
        {
            std::cout << "illegal position";
        }
        auto tmpBoard = allPieceMoves(actualPiecePos);

        if (tmpBoard != 0)
        {
            int endPos;
            while ((endPos = nextSquare(&tmpBoard)) < NUMBER_OF_SQUARES)
            {
                Piece movingPiece = getPieceAt((Position)actualPiecePos);
                currentMove->start = (Position)actualPiecePos;
                currentMove->end = (Position)endPos;
                currentMove->captured = getPieceAt((Position)endPos);

                auto piecerank = rank(currentMove->end);

                memcpy(&tempBoard, &state->boards[0], sizeof(uint64_t) * NUMBER_OF_PIECES);

                // this move is an enpassant move?
                if ((movingPiece == P || movingPiece == p) && currentMove->end == state->enpassant)
                {
                    currentMove->enpassant = state->enpassant;
                    auto hitPosition = (state->color == BLACK) ? state->enpassant + 8 : state->enpassant - 8;
                    currentMove->captured = getPieceAt((Position)hitPosition);
                    auto &capturedBoard = state->boards[currentMove->captured];
                    capturedBoard &= ~(1LLU << hitPosition);
                }

                // do move
                auto &movingBoard = state->boards[movingPiece];

                if (currentMove->captured != NUMBER_OF_PIECES)
                {
                    auto &capturedBoard = state->boards[currentMove->captured];
                    // normal capture - remove captured piece
                    capturedBoard &= ~(1LLU << currentMove->end);
                }
                // move a general piece
                movingBoard ^= (1LLU << currentMove->start);
                movingBoard |= 1LLU << currentMove->end;

                auto oppositeKingPos = state->color == WHITE ? getPiecePos(K) : getPiecePos(k);

                // is in check?
                if (!isSqareAttacked(oppositeKingPos, &state->boards[0], oppositeColor(state->color)))
                {
                    // Promotion?
                    // white pawn on the rank H - black pawn on the rank A?
                    if ((piecerank == 7 && movingPiece == P) |
                        (piecerank == 0 && movingPiece == p))
                    {
                        Piece captured = currentMove->captured;
                        // generate all the 4 possible promotions
                        currentMove->promotion = (Piece)(R + state->color);
                        currentMove->captured = captured;
                        currentMove->enpassant = NUMBER_OF_SQUARES;
                        currentMove++;
                        currentMove->start = (Position)actualPiecePos;
                        currentMove->end = (Position)endPos;
                        currentMove->promotion = (Piece)(N + state->color);
                        currentMove->captured = captured;
                        currentMove->enpassant = NUMBER_OF_SQUARES;
                        currentMove++;
                        currentMove->start = (Position)actualPiecePos;
                        currentMove->end = (Position)endPos;
                        currentMove->promotion = (Piece)(B + state->color);
                        currentMove->captured = captured;
                        currentMove->enpassant = NUMBER_OF_SQUARES;
                        currentMove++;
                        currentMove->start = (Position)actualPiecePos;
                        currentMove->end = (Position)endPos;
                        currentMove->promotion = (Piece)(Q + state->color);
                        currentMove->captured = captured;
                        currentMove->enpassant = NUMBER_OF_SQUARES;
                    }
                    // save currentmove
                    currentMove++;
                }
                else
                {
                    perftChecks++;
                }
                // restore board
                memcpy(&state->boards[0], &tempBoard, sizeof(uint64_t) * NUMBER_OF_PIECES);
            }
        }
    }
    // casting is possible?
    state->color == WHITE ? board = getWhitePiecesBoard() : board = getBlackPiecesBoard();

    if (state->castling != 0 && state->color == WHITE && !isSqareAttacked(getPiecePos(K), &state->boards[0], oppositeColor(state->color)))
    {
        if (state->castling & CASTLING_WHITE_KINGSIDE)
        {
            // king on e1, rook on h1 and f1 and g1 empty
            // king on f1, g1 not in check
            if (getPiecePos(K) == e1 && getPieceAt(h1) == R &&
                getPieceAt(f1) == NUMBER_OF_PIECES && getPieceAt(g1) == NUMBER_OF_PIECES &&
                !isSqareAttacked(e1, &state->boards[0], oppositeColor(state->color)) &&
                !isSqareAttacked(f1, &state->boards[0], oppositeColor(state->color)) &&
                !isSqareAttacked(g1, &state->boards[0], oppositeColor(state->color)))
            {
                currentMove->start = e1;
                currentMove->end = g1;
                currentMove->promotion = NUMBER_OF_PIECES;
                currentMove->captured = NUMBER_OF_PIECES;
                currentMove->enpassant = NUMBER_OF_SQUARES;
                currentMove++;
            }
        }
        if (state->castling & CASTLING_WHITE_QUEENSIDE)
        {
            // king on e1, rook on a1, b1 c1 d1 empty
            // king on e1, d1, c1 not in check
            if (getPiecePos(K) == e1 && getPieceAt(a1) == R &&
                getPieceAt(b1) == NUMBER_OF_PIECES && getPieceAt(c1) == NUMBER_OF_PIECES && getPieceAt(d1) == NUMBER_OF_PIECES &&
                !isSqareAttacked(e1, &state->boards[0], oppositeColor(state->color)) &&
                !isSqareAttacked(d1, &state->boards[0], oppositeColor(state->color)) &&
                !isSqareAttacked(c1, &state->boards[0], oppositeColor(state->color)))
            {
                currentMove->start = e1;
                currentMove->end = c1;
                currentMove->promotion = NUMBER_OF_PIECES;
                currentMove->captured = NUMBER_OF_PIECES;
                currentMove->enpassant = NUMBER_OF_SQUARES;
                currentMove++;
            }
        }
    }

    if (state->castling != 0 && state->color == BLACK && !isSqareAttacked(getPiecePos(k), &state->boards[0], oppositeColor(state->color)))
    {
        if (state->castling & CASTLING_BLACK_KINGSIDE)
        {
            // king on e8, rook on h8 and f8 and g8 are empty
            // king on f8, g8 not in check
            if (getPiecePos(k) == e8 && getPieceAt(h8) == r &&
                getPieceAt(f8) == NUMBER_OF_PIECES && getPieceAt(g8) == NUMBER_OF_PIECES &&
                !isSqareAttacked(e8, &state->boards[0], oppositeColor(state->color)) &&
                !isSqareAttacked(f8, &state->boards[0], oppositeColor(state->color)) &&
                !isSqareAttacked(g8, &state->boards[0], oppositeColor(state->color)))
            {
                currentMove->start = e8;
                currentMove->end = g8;
                currentMove->promotion = NUMBER_OF_PIECES;
                currentMove->captured = NUMBER_OF_PIECES;
                currentMove->enpassant = NUMBER_OF_SQUARES;
                currentMove++;
            }
        }
        if (state->castling & CASTLING_BLACK_QUEENSIDE)
        {
            // king on e8, rook on a8, b8 c8 d8 are empty
            // king on e1, d1, c1 not in check
            if (getPiecePos(k) == e8 && getPieceAt(a8) == r &&
                getPieceAt(b8) == NUMBER_OF_PIECES && getPieceAt(c8) == NUMBER_OF_PIECES && getPieceAt(d8) == NUMBER_OF_PIECES &&
                !isSqareAttacked(e8, &state->boards[0], oppositeColor(state->color)) &&
                !isSqareAttacked(d8, &state->boards[0], oppositeColor(state->color)) &&
                !isSqareAttacked(c8, &state->boards[0], oppositeColor(state->color)))
            {
                currentMove->start = e8;
                currentMove->end = c8;
                currentMove->promotion = NUMBER_OF_PIECES;
                currentMove->captured = NUMBER_OF_PIECES;
                currentMove->enpassant = NUMBER_OF_SQUARES;
                currentMove++;
            }
        }
    }
    moves.length = (int)(currentMove - startMove);
    return moves;
}

bool ChessBoard::isSqareAttacked(Position square, uint64_t *board, Color oppositeColor)
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
Position ChessBoard::getPiecePos(Piece piece)
{
    // select board
    auto board = state->boards[piece];
    // determinate the first bit
    auto index = bitScan64(board);
    return (Position)index;
}

uint64_t ChessBoard::allPieceMoves(Position pos)
{
    Piece piece = getPieceAt((Position)pos);

    uint64_t notOwnPieces;
    uint64_t oppositePieces;
    Position oppositeKingPos;

    switch (piece)
    {
    case N:
    case n:
        notOwnPieces = (state->color == WHITE ? ~getWhitePiecesBoard() : ~getBlackPiecesBoard());
        return knightMoves(pos) & notOwnPieces;
    case B:
    case b:
        notOwnPieces = (state->color == WHITE ? ~getWhitePiecesBoard() : ~getBlackPiecesBoard());
        return bishopMoves(pos) & notOwnPieces;
    case K:
    case k:
        notOwnPieces = (state->color == WHITE ? ~getWhitePiecesBoard() : ~getBlackPiecesBoard());
        oppositeKingPos = state->color == BLACK ? getPiecePos(K) : getPiecePos(k);
        return (kingMoves(pos) & ~kingMoves(oppositeKingPos)) & notOwnPieces;
    case P:
        oppositePieces = state->color == BLACK ? getWhitePiecesBoard() : getBlackPiecesBoard();
        return pawnWhiteMoves(pos) | (pawnWhiteHitMoves(pos) & oppositePieces);
    case p:
        oppositePieces = state->color == BLACK ? getWhitePiecesBoard() : getBlackPiecesBoard();
        return pawnBlackMoves(pos) | (pawnBlackHitMoves(pos) & oppositePieces);
    case R:
    case r:
        notOwnPieces = (state->color == WHITE ? ~getWhitePiecesBoard() : ~getBlackPiecesBoard());
        return rookMoves(pos) & notOwnPieces;
    case Q:
    case q:
        notOwnPieces = (state->color == WHITE ? ~getWhitePiecesBoard() : ~getBlackPiecesBoard());
        return (rookMoves(pos) | bishopMoves(pos)) & notOwnPieces;
    default:
        return 0;
    }
}

uint64_t ChessBoard::rookMoves(Position pos)
{
    auto allPiecesAsBitmap = getWhitePiecesBoard() | getBlackPiecesBoard();
    auto removeRookAtPosition = ~(1LLU << (Position)pos);

    uint64_t rank = rankAttack(allPiecesAsBitmap & removeRookAtPosition, (Position)pos);
    uint64_t file = fileAttack(allPiecesAsBitmap & removeRookAtPosition, (Position)pos);
    return rank | file;
}

uint64_t ChessBoard::bishopMoves(Position pos)
{
    auto allPiecesAsBitmap = getWhitePiecesBoard() | getBlackPiecesBoard();
    auto notBishopAtPosition = ~(1LLU << (Position)pos);

    uint64_t diagonal = diagonalAttack((allPiecesAsBitmap & notBishopAtPosition), (Position)pos);

    uint64_t antiDiagonal = antidiagonalAttack(allPiecesAsBitmap & notBishopAtPosition, (Position)pos);

    return diagonal | antiDiagonal;
}

uint64_t ChessBoard::knightMoves(Position pos)
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

uint64_t ChessBoard::pawnBlackMoves(Position pos)
{
    uint64_t tmp = 1LLU << pos;
    // move south if there is not any pieces in front of the pawn
    uint64_t ret = SOUTH(tmp) & ~(getWhitePiecesBoard() | getBlackPiecesBoard());

    // if pawn can move 1 square, we can try move 2 squares
    // if it is on the second row (0xff00000000)
    if (ret)
    {
        ret |= SOUTH_TWO(tmp) & 0xff00000000 & ~(getWhitePiecesBoard() | getBlackPiecesBoard());
    }
    // enpassant
    if (state->enpassant != NUMBER_OF_SQUARES)
    {
        uint64_t entP = 1LLU << state->enpassant;
        // move south if there is not any pieces in front of the pawn
        ret |= ((SOUTH_EAST(tmp) & notHFile) | (SOUTH_WEST(tmp) & notAFile)) & entP;
    }
    return ret;
}

uint64_t ChessBoard::pawnBlackHitMoves(Position pos)
{
    uint64_t tmp = 1LLU << pos;
    // hit south east and west if possible
    uint64_t ret = ((SOUTH_EAST(tmp) & notHFile) | (SOUTH_WEST(tmp) & notAFile));
    return ret;
}

uint64_t ChessBoard::pawnWhiteHitMoves(Position pos)
{
    uint64_t tmp = 1LLU << pos;
    // move south if there is not any pieces in front of the pawn
    uint64_t ret = ((NORTH_EAST(tmp) & notHFile) | (NORTH_WEST(tmp) & notAFile));
    return ret;
}

uint64_t ChessBoard::pawnWhiteMoves(Position pos)
{
    uint64_t tmp = 1LLU << pos;
    // move north if there is not any pieces in front of the pawn
    uint64_t ret = NORTH(tmp) & ~(getWhitePiecesBoard() | getBlackPiecesBoard());

    // if pawn can move 1 square, we can try move 2 squares
    // if it is on the second row (0xff000000)
    if (ret)
    {
        ret |= NORTH_TWO(tmp) & 0xff000000 & ~(getWhitePiecesBoard() | getBlackPiecesBoard());
    }
    // enpassant
    if (state->enpassant != NUMBER_OF_SQUARES)
    {
        uint64_t entP = 1LLU << state->enpassant;
        // move north if there is not any pieces in front of the pawn
        ret |= ((NORTH_EAST(tmp) & notHFile) | (NORTH_WEST(tmp) & notAFile)) & entP;
    }
    return ret;
}

uint64_t ChessBoard::kingMoves(Position pos)
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

std::string ChessBoard::showOneBitBoard(uint64_t board, int startPos, int endPos)
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

std::string ChessBoard::to_string(int startPos, int endPos)
{
    std::string out;
    int rowNumber = endPos / 8;
    int boardPos = 8 * rowNumber;

    for (int i = endPos; i >= startPos; i--)
    {
        bool found = false;
        for (int j = 0; j < NUMBER_OF_PIECES; j++)
        {
            if (state->boards[j] & (1LLU << boardPos))
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

int ChessBoard::generate_rank_attack(int o, int f)
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

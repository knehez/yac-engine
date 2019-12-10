#include "board.h"

Board::Board()
{
    clear();

    for (int x = a1; x < h8; x++)
    {
        // http://cinnamonchess.altervista.org/bitboard_calculator/Calc.html

        for (int f = 0; f < 8; f++)
        {
            RANK_ATTACK[x * 8 + f] = generate_rank_attack(x * 2, f);
        }
        int file = x & 7;
        int rank = x >> 3;

        MASK[x].file = FILES[file];

        MASK[x].diagonal = 0;
        // file diagonal bits UP
        for (int i = x; i <= h8; i += 9)
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

void Board::clear()
{
    for (int i = 0; i < NumOfPieces; i++)
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
    for (int i = 0; i < NumOfPieces; i++)
    {
        if (piece == m_pieces[i])
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
uint64_t Board::attack(uint64_t pieces, Position pos, uint64_t mask)
{
    uint64_t o = pieces & mask;
    uint64_t r = bit_bswap(o);

    return ((o - (1ULL << pos)) ^ bit_bswap(r - (1ULL << (pos ^ 56)))) & mask;
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
    for (int i = 0; i < NumOfPieces; i++)
    {
        if (piece == m_pieces[i])
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
        for (int j = 0; j < NumOfPieces; j++)
        {
            if (m_boards[j] & (1LLU << boardPos))
            {
                fen += m_pieces[j];
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

void Board::setFENCode(std::string fenCode)
{
    int rowNumber = 7;
    int boardPos = 8 * rowNumber;

    for (char &c : fenCode)
    {
        int emptyPositions = atoi(&c);
        if (emptyPositions != 0)
        {
            for (int i = 0; i < emptyPositions; i++)
            {
                for (int j = 0; j < NumOfPieces; j++)
                {
                    // set empty position of all the boards
                    m_boards[j] &= ~(1LLU << boardPos);
                }
                boardPos++;
            }
            continue;
        }
        for (int i = 0; i < NumOfPieces; i++)
        {
            if (c == m_pieces[i])
            {
                m_boards[i] |= (1LLU << boardPos);
                break;
            }
        }
        if (c == '/')
        {
            rowNumber--;
            boardPos = 8 * rowNumber;
            continue;
        }

        if (c == ' ')
        {
            break;
        }
        boardPos++;
    }
    // w or b after the first space
    auto index = fenCode.find(' ');
    if (index != std::string::npos)
    {
        fenCode[index + 1] == 'w' ? color = WHITE : color = BLACK;
    }
}

char Board::getPiece(int pos)
{
    for (int j = 0; j < NumOfPieces; j++)
    {
        if (m_boards[j] & (1LLU << pos))
        {
            return m_pieces[j];
        }
    }
    return '-';
}

uint64_t Board::allPossibleMoves(int pos)
{
    char piece = getPiece(pos);

    switch (piece)
    {
    case 'N':
    case 'n':
        return knightMoves(pos);
    case 'B':
    case 'b':
        return bishopMoves(pos);
    case 'K':
    case 'k':
        return kingMoves(pos);
    case 'P':
    case 'p':
        // return pawn;
    case 'R':
    case 'r':
        return rookMoves(pos);
    case 'Q':
    case 'q':
        return rookMoves(pos) | bishopMoves(pos);
    default:
        return 0;
    }
}

uint64_t Board::rookMoves(int pos)
{
    auto allPiecesAsBitmap = getWhitePiecesBoard() | getBlackPiecesBoard();
    auto removeRookAtPosition = ~(1LLU << (Position)pos);

    uint64_t rank = rankAttack(allPiecesAsBitmap & removeRookAtPosition, (Position) pos);
    uint64_t file = fileAttack(allPiecesAsBitmap & removeRookAtPosition, (Position) pos);
    return (rank | file) & (color == WHITE ? ~getWhitePiecesBoard() : ~getBlackPiecesBoard());
}

uint64_t Board::bishopMoves(int pos)
{
    auto allPiecesAsBitmap = getWhitePiecesBoard() | getBlackPiecesBoard();
    auto notBishopAtPosition = ~(1LLU << (Position)pos);

    uint64_t diagonal = diagonalAttack((allPiecesAsBitmap & notBishopAtPosition), (Position)pos);

    uint64_t antiDiagonal = antidiagonalAttack(allPiecesAsBitmap & notBishopAtPosition, (Position)pos);

    return (diagonal | antiDiagonal) & (color == WHITE ? ~getWhitePiecesBoard() : ~getBlackPiecesBoard());
}

uint64_t Board::knightMoves(int pos)
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
    return ret & (color == WHITE ? ~getWhitePiecesBoard() : ~getBlackPiecesBoard());
}

uint64_t Board::kingMoves(int pos)
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
    return ret & (color == WHITE ? ~getWhitePiecesBoard() : ~getBlackPiecesBoard());
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
        for (int j = 0; j < NumOfPieces; j++)
        {
            if (m_boards[j] & (1LLU << boardPos))
            {
                out += m_pieces[j];
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

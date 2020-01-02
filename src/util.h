#pragma once

#include <stdint.h>
#include <string>

#define MAX_BOARD_MOVEMENTS 1024

const uint64_t notAFile = 0X7F7F7F7F7F7F7F7F;
const uint64_t notHFile = 0XFEFEFEFEFEFEFEFE;
const uint64_t notABFile = 0X3f3f3f3f3f3f3f3f;
const uint64_t notGHFile = 0XFCFCFCFCFCFCFCFC;

const uint64_t A_FILE = 0x101010101010101;
const uint64_t B_FILE = 0x202020202020202;
const uint64_t C_FILE = 0x404040404040404;
const uint64_t D_FILE = 0x808080808080808;
const uint64_t E_FILE = 0x1010101010101010;
const uint64_t F_FILE = 0x2020202020202020;
const uint64_t G_FILE = 0x4040404040404040;
const uint64_t H_FILE = 0x8080808080808080;

const uint64_t FILES[8] = {A_FILE, B_FILE, C_FILE, D_FILE, E_FILE, F_FILE, G_FILE, H_FILE};

#define NORTH(b) b << 8
#define NORTH_WEST(b) b << 7
#define NORTH_EAST(b) b << 9
#define WEST(b) b >> 1
#define EAST(b) b << 1
#define SOUTH(b) b >> 8
#define SOUTH_WEST(b) b >> 9
#define SOUTH_EAST(b) b >> 7

#define NORTH_TWO(b) b << 16
#define SOUTH_TWO(b) b >> 16

#define KNIGHT_NORTH_EAST(b) b << 17
#define KNIGHT_NORTH_WEST(b) b << 15
#define KNIGHT_EAST_NORTH(b) b << 10
#define KNIGHT_EAST_SOUTH(b) b >> 10
#define KNIGHT_WEST_NORTH(b) b << 6
#define KNIGHT_WEST_SOUTH(b) b >> 6
#define KNIGHT_SOUTH_EAST(b) b >> 15
#define KNIGHT_SOUTH_WEST(b) b >> 17

int countBits(uint64_t b);
uint64_t bit_bswap(uint64_t board);
int countPiecesOnBoard(uint64_t board);
void trimWhiteSpaces(std::string &str);

#define algebraicFile(pos) 'a' + (pos & 7)
#define algebraicRank(pos) '1' + (pos >> 3)
#define oppositeColor(c) c == WHITE ? BLACK : WHITE

#define rank(p) (p >> 3);

double chrono(void);

enum Castling
{
    CASTLING_WHITE_KINGSIDE = 1,
    CASTLING_WHITE_QUEENSIDE = 2,
    CASTLING_BLACK_KINGSIDE = 4,
    CASTLING_BLACK_QUEENSIDE = 8
};

typedef struct Mask
{
    uint64_t diagonal;
    uint64_t antidiagonal;
    uint64_t file;
    uint64_t knight;
    uint64_t king;
    uint64_t pawnBlackHits;
    uint64_t pawnWhiteHits;
} Mask;

enum Color
{
    WHITE,
    BLACK,
    NUMBER_OF_COLORS
};

const char piecesChars[] = "PpRrNnBbQqKk";

enum Piece
{
    P,
    p,
    R,
    r,
    N,
    n,
    B,
    b,
    Q,
    q,
    K,
    k,
    NUMBER_OF_PIECES
};

enum Position
{
    a1,
    b1,
    c1,
    d1,
    e1,
    f1,
    g1,
    h1,
    a2,
    b2,
    c2,
    d2,
    e2,
    f2,
    g2,
    h2,
    a3,
    b3,
    c3,
    d3,
    e3,
    f3,
    g3,
    h3,
    a4,
    b4,
    c4,
    d4,
    e4,
    f4,
    g4,
    h4,
    a5,
    b5,
    c5,
    d5,
    e5,
    f5,
    g5,
    h5,
    a6,
    b6,
    c6,
    d6,
    e6,
    f6,
    g6,
    h6,
    a7,
    b7,
    c7,
    d7,
    e7,
    f7,
    g7,
    h7,
    a8,
    b8,
    c8,
    d8,
    e8,
    f8,
    g8,
    h8,
    NUMBER_OF_SQUARES
};

Position bitScan64(uint64_t board);

typedef struct move
{
    Position start;
    Position end;
    Piece promotion = NUMBER_OF_PIECES;

    Piece captured = NUMBER_OF_PIECES;
    Position enpassant = NUMBER_OF_SQUARES;
} Move;

enum
{
    MAX_MOVES = 256
};

typedef struct Moves {
	Move move[MAX_MOVES];
	int length = 0;
} Moves;

std::string to_string(Moves moves); 
void removeOneMove(Moves &moves, Move move);

#define NULL_MOVE {NUMBER_OF_SQUARES, NUMBER_OF_SQUARES, NUMBER_OF_PIECES, NUMBER_OF_PIECES, NUMBER_OF_SQUARES}
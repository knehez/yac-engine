#pragma once
#include <intrin.h>

const uint64_t notAFile = 0X7F7F7F7F7F7F7F7F;
const uint64_t notHFile = 0XFEFEFEFEFEFEFEFE;
const uint64_t notABFile = 0X3f3f3f3f3f3f3f3f;
const uint64_t notGHFile = 0XFCFCFCFCFCFCFCFC;

const uint64_t AFILE = 0x1010101010101010;
const uint64_t BFILE = 0x2020202020202020;
const uint64_t CFILE = 0x4040404040404040;
const uint64_t DFILE = 0x808080808080808;
const uint64_t EFILE = 0x1010101010101010;
const uint64_t FFILE = 0x2020202020202020;
const uint64_t GFILE = 0x4040404040404040;
const uint64_t HFILE = 0x8080808080808080;

const uint64_t FILES[8] = {AFILE, BFILE, CFILE, DFILE, EFILE, FFILE, GFILE, HFILE};

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

#define countBits(b) __popcnt64(b)
#define bit_bswap(b) _byteswap_uint64(b)
#define algebraicFile(pos) 'a' + (pos & 7)
#define algebraicRank(pos) '1' + (pos >> 3)

typedef struct Mask
{
    uint64_t diagonal;
    uint64_t antidiagonal;
    uint64_t file;
} Mask;

enum Color
{
    WHITE,
    BLACK
};

const char piecesChars[] = "PpRrNnBbQqKk";

enum Pieces
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
    NumOfPieces
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
    h8
};
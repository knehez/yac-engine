#include "util.h"
#include <time.h>
#include <string>

#if _WIN64
#include <intrin.h>
#include <Windows.h>
#elif __linux__
#include <unistd.h>
#include <sys/time.h>
#endif

double chrono(void)
{
#if defined(__linux__)
#if _POSIX_TIMERS > 0
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return 0.000000001 * t.tv_nsec + t.tv_sec;
#else
    struct timeval t;
    gettimeofday(&t, NULL);
    return 0.000001 * t.tv_usec + t.tv_sec;
#endif
#elif _WIN64
    return 0.001 * GetTickCount();
#endif
}

int countPiecesOnBoard(uint64_t board)
{
#if defined(_WIN64)
    return static_cast<int>(__popcnt64(board));
#elif __linux__
    return static_cast<int>(__builtin_popcountll(board));
#endif
}

Position bitScan64(uint64_t board)
{
#if _WIN64
    unsigned long index;
    _BitScanForward64(&index, board);
    return static_cast<Position>(index);
#elif __linux__
    return static_cast<Position>(__builtin_ctzll(board));
#endif
}

uint64_t bit_bswap(uint64_t board)
{
#if _WIN64
    return _byteswap_uint64(board);
#elif __linux__
    return __builtin_bswap64(board);
#endif
}

void trimWhiteSpaces(std::string &str)
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

std::string to_string(Moves moves)
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

bool operator==(const Move &lmove, const Move &rmove)
{
    if (lmove.start == rmove.start &&
        lmove.end == rmove.end &&
        lmove.captured == rmove.captured &&
        lmove.enpassant == rmove.enpassant &&
        lmove.promotion == rmove.promotion)
    {
        return true;
    }
    return false;
}

void removeOneMove(Moves &moves, Move move)
{
    int i = 0;
    for (; i < moves.length; i++)
    {
        if (moves.move[i] == move)
        {
            break;
        }
    }

    if (i == moves.length)
    {
        // item not found
        return;
    }

    if (i == moves.length - 1)
    {
        // item is the last element
        moves.length--;
        return;
    }

    // quick remove: swap item with the last one
    moves.move[i] = moves.move[moves.length - 1];
    moves.length--;
}
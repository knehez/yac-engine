#include "util.h"
#include <time.h>

#if _WIN64
#include <intrin.h>
#include <windows.h>
#elif __linux__
#include <unistd.h>
#include <sys/time.h>
#endif

double chrono(void)
{
#if __linux__
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
    return (int)__popcnt64(board);
#elif __linux__
    return __builtin_popcountll(board);
#endif
}

Position bitScan64(uint64_t board)
{
#if _WIN64
    unsigned long index;
    _BitScanForward64(&index, board);
    return (Position)index;
#elif __linux__
    return (Position)__builtin_ctzll(board);
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
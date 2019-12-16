#include "constants.h"

#if _WIN64
#include <intrin.h>
#elif __linux__

#endif

int countPiecesOnBoard(uint64_t board)
{
#if _WIN64
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
    return __builtin_ctzll(board);
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
#include <iostream>
#include "board.h"

int main()
{
	Board board;
	// board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

	auto b1 = board.diagonalAttack(0x8000200000000200, d4);

	std::cout << "Board 2: \n" << board.showOneBitBoard(b1) << "\n";
    return 0;
}
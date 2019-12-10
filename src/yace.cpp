#include <iostream>
#include "board.h"

int main()
{
	// complex move from starting position
	Board board;
	board.setFENCode("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");

	auto b = board.allPossibleMoves(f1);
	auto str = board.showOneBitBoard(b, a1, h8);
	std::cout << str;
	return 0;
}
#include <iostream>
#include "board.h"

int main()
{
	double time = -chrono();

	// complex move from starting position
	Board board;
	board.setFENCode("rnbqkbnr/p1pppp1p/1p6/P5p1/7P/8/1PPPPPP1/RNBQKBNR w KQkq - 0 1");
	board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

	std::cout << board.getMoves() << "\n";
	
	time += chrono();
	printf("%10.3f sec\n", time);

	return 0;
}
#include <iostream>
#include "chessboard.h"

int main()
{
	double time = -chrono();

	// complex move from starting position
	ChessBoard board;
	const char* fenCode = "rnbqkbnr/p1pppp1p/1p6/P5p1/7P/8/1PPPPPP1/RNBQKBNR w KQkq - 0 1";
	board.setFENCode(fenCode);

	// std::cout << board.generateMoves(m) << "\n";
	
	time += chrono();
	printf("%10.3f sec\n", time);

	return 0;
}
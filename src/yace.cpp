#include <iostream>
#include "chessboard.h"

int main()
{
	ChessBoard board;
	board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

	double time = -chrono();

	auto count = board.perft(6);
	board.fenFile.close();

	time += chrono();

	printf("Number of captures: %llu\n", board.perftCaptures);
	printf("Number of checks: %llu\n", board.perftChecks);
	printf("%llu moves %10.3f sec - %f moves/sec \n", count, time, count / time);

	return 0;
}
#include <iostream>
#include "board.h"

int main()
{
	// complex move from starting position
    Board board;
    board.setFENCode("rnbqkbnr/p1pppp1p/1p6/P5p1/7P/8/1PPPPPP1/RNBQKBNR w KQkq - 0 1");

    auto b = board.allPossibleMoves(a5);
	b = b | board.allPossibleMoves(h4);
	auto str = board.showOneBitBoard(b, a1, h8);
    
	std::cout << str;
	return 0;
}
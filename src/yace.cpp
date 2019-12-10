#include <iostream>
#include "board.h"

int main()
{
	// complex move from starting position
    Board board;
    board.setFENCode("rnb1kbnr/p1pp1ppp/8/1p4p1/3B1q2/8/PPPPPPPP/RNBQK1N1 b Qkq - 2 2");

    auto b = board.allPossibleMoves(f4);
	auto str = board.showOneBitBoard(b, a1, h8);

	std::cout << str;
	return 0;
}
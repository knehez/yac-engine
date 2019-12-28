#include <iostream>
#include "chessboard.h"
#include <time.h>
#include <algorithm>
#include "engine.h"

int monteCarloSearch(const char *fen);

ChessBoard board;
void game();

int main()
{
	srand((unsigned int)time(NULL));
	Engine engine;
	engine.start();
	return 0;
}


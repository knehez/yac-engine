#include <iostream>
#include "chessboard.h"
#include <time.h>
#include <algorithm>
#include "engine.h"
#include <string>

int monteCarloSearch(const char *fen);

ChessBoard board;
void game();
void perft();

int main(int argc, char **argv)
{
	for (int i = 0; i < argc; i++)
	{
		if (std::string(argv[i]) == "perft")
		{
			perft();
			return 0;
		}
	}

	srand((unsigned int)time(NULL));
	Engine engine;
	engine.start();
	return 0;
}

void perft()
{
	auto start = std::chrono::high_resolution_clock::now();
	Engine engine;
	engine.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	auto p = engine.perft(6);
	auto endtime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = endtime - start;
	std::cout << "time: " << elapsed.count();
}

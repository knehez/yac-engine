#include <iostream>
#include "chessboard.h"
#include <time.h>
#include <algorithm>

int monteCarloSearch(const char *fen);

ChessBoard board;
void game();

int main()
{
	srand(time(NULL));

	game();
	return 0;
	ChessBoard board;
	board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

	double time = -chrono();

	auto count = board.perft(6);
	board.fenFile.close();

	time += chrono();

	printf("\nNumber of captures: %llu\n", board.perftCaptures);
	printf("Number of checks: %llu\n", board.perftChecks);
	printf("%llu moves %10.3f sec - %f moves/sec \n", count, time, count / time);

	return 0;
}

void parseMove(std::string move, Move *m)
{
	m->start = (Position)(((move[1] - '1') << 3) + (move[0] - 'a'));
	m->end = (Position)(((move[3] - '1') << 3) + (move[2] - 'a'));
}

void game()
{
	board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

	std::cout << "You are White.\n";

	std::string strMove;

	int index = 0;

	std::vector<std::string> vv = {"e2e4", "g1f3", "f3d6"};
	while (true)
	{
		Move move;
		do
		{
			std::cout << "Your next move:" << "\n";
			std::cin >> strMove;
			parseMove(strMove, &move);		
		} while(board.validateMove(&move) == false);

		board.move(move);

		Moves moves;
		moves = board.generateMoves(moves);
		std::vector<int> v;
		for (int i = 0; i < moves.length; i++)
		{
			board.move(moves.move[i]);
			v.push_back(monteCarloSearch(board.getFENCode().c_str()));
			board.undoMove(moves.move[i]);
		}
		int maxElementIndex = std::max_element(v.begin(), v.end()) - v.begin();
		std::cout << "\nBest move value:" << v[maxElementIndex];
		std::cout << "\nYaChess move:" << board.print_move(moves.move[maxElementIndex]);
		board.move(moves.move[maxElementIndex]);
		std::cout << "\n********\n" << board.to_string() << "\n********\n";
	}

	auto count = monteCarloSearch("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

int monteCarloSearch(const char *fen)
{
	uint64_t count = 0;
	int sumValue = 0;
	int value = 0;
	bool isMateFound = false;
	for (int i = 0; i < 1000; i++)
	{
		count += board.monteCarloSimulation(12, isMateFound, value);
		if (board.getFENCode() != fen)
		{
			std::cout << "******************** DIFFERENT FEN! **************************";
		}
		if (isMateFound)
		{
			sumValue += value;
			value = 0;
			isMateFound = false;
			board.setFENCode(fen);
		}
	}
	return sumValue;
}
#include<iostream>
#include<cstring>
#include<cstdlib>
#include<ctime>
#include"Solitaire.h"
#define _stricmp strcasecmp

using namespace std;

//A variation of the Klondike solver that deals a game by its FreeCell (FC) game
//number and plays it with a "simple method" rather than an exhaustive search.
//
//The simple method is a greedy, human-like strategy: it never draws from the
//stock while there is any other move available (a move between tableau columns,
//a move to a foundation, or a move of the current draw card onto a tableau
//column). It only flips cards from the stock once nothing else can be played.
//Because it never backtracks it is fast but will not solve every deal.

int main(int argc, char * argv[]) {
	Solitaire s;
	s.Initialize();

	int gameNumber = 1;
	int drawCount = 1;
	bool showMoves = false;
	bool replay = false;

	for (int i = 1; i < argc; i++) {
		if (_stricmp(argv[i], "-game") == 0 || _stricmp(argv[i], "/game") == 0 || _stricmp(argv[i], "-g") == 0 || _stricmp(argv[i], "/g") == 0) {
			if (i + 1 >= argc) { cout << "You must specify an FC game number to load.\n"; return 0; }
			gameNumber = atoi(argv[++i]);
		} else if (_stricmp(argv[i], "-draw") == 0 || _stricmp(argv[i], "/draw") == 0 || _stricmp(argv[i], "-dc") == 0 || _stricmp(argv[i], "/dc") == 0) {
			if (i + 1 >= argc) { cout << "You must specify draw count.\n"; return 0; }
			drawCount = atoi(argv[++i]);
			if (drawCount < 1 || drawCount > 12) { cout << "Please specify a valid draw count from 1 to 12.\n"; return 0; }
		} else if (_stricmp(argv[i], "-mvs") == 0 || _stricmp(argv[i], "/mvs") == 0 || _stricmp(argv[i], "-moves") == 0 || _stricmp(argv[i], "/moves") == 0) {
			showMoves = true;
		} else if (_stricmp(argv[i], "-r") == 0 || _stricmp(argv[i], "/r") == 0) {
			replay = true;
		} else if (_stricmp(argv[i], "-?") == 0 || _stricmp(argv[i], "/?") == 0 || _stricmp(argv[i], "?") == 0 || _stricmp(argv[i], "/help") == 0 || _stricmp(argv[i], "-help") == 0) {
			cout << "SimpleSolver\n";
			cout << "Deals a Klondike game by FreeCell (FC) game number and plays it with a\n";
			cout << "simple greedy method that never draws from stock while another move exists.\n\n";
			cout << "SimpleSolver [/G #] [/DC #] [/MVS] [/R] [#]\n\n";
			cout << "  /GAME # [/G #]    FC game number to deal and solve. Defaults to 1.\n";
			cout << "  /DRAW # [/DC #]   Draw count to use. Defaults to 1.\n";
			cout << "  /MOVES [/MVS]     Output the compact list of moves made when solved.\n";
			cout << "  /R                Replay the solution step by step to output.\n";
			cout << "  #                 A bare number is treated as the FC game number.\n";
			return 0;
		} else {
			gameNumber = atoi(argv[i]);
		}
	}

	s.SetDrawCount(drawCount);
	s.ShuffleFC(gameNumber);
	s.ResetGame(drawCount);

	cout << "FC Game #" << gameNumber << " (draw " << drawCount << ")\n";
	cout << s.GameDiagram() << "\n\n";

	clock_t start = clock();
	SolveResult result = s.SolveSimple(500);
	clock_t elapsed = clock() - start;

	bool solved = (result == SolvedMinimal || result == SolvedMayNotBeMinimal);
	if (solved) {
		cout << "Solved with the simple method in " << s.MovesMadeNormalizedCount() << " moves.";
	} else {
		cout << "Not solved by the simple method. Best foundation count " << s.FoundationCount() << "/52 after " << s.MovesMadeNormalizedCount() << " moves.";
	}
	cout << " Took " << elapsed << " ms.\n";

	if (solved && showMoves) {
		cout << "\n" << s.MovesMade() << "\n";
	}

	if (solved && replay) {
		int movesToMake = s.MovesMadeCount();
		s.ResetGame(drawCount);
		for (int i = 0; i < movesToMake; i++) {
			cout << "----------------------------------------\n";
			cout << s.GetMoveInfo(s[i]) << "\n\n";
			s.MakeMove(s[i]);
			cout << s.GameDiagram() << "\n\n";
		}
		cout << "----------------------------------------\n";
	}

	return 0;
}

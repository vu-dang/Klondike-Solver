#include<iostream>
#include<cstring>
#include<cstdlib>
#include<ctime>
#include"Solitaire.h"
#define _stricmp strcasecmp

using namespace std;

//A variation of the Klondike solver that deals a game by its FreeCell (FC) game
//number and solves it with a "simple method".
//
//The simple method is a full exhaustive search, exactly like the minimal solver,
//with a single added rule: it is not allowed to draw from the stock while any other
//move is available (a move between tableau columns, a move to a foundation, or a move
//of the current draw card onto a tableau column). When one or more such moves exist
//every one of them is still explored as its own branch; only once nothing else can be
//played is the solver permitted to flip cards from the stock. It therefore finds the
//minimal solution obeying that constraint, or reports the deal as impossible under it.

int main(int argc, char * argv[]) {
	Solitaire s;
	s.Initialize();

	int gameNumber = 1;
	int drawCount = 1;
	int maxStates = 5000000;
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
		} else if (_stricmp(argv[i], "-states") == 0 || _stricmp(argv[i], "/states") == 0 || _stricmp(argv[i], "-s") == 0 || _stricmp(argv[i], "/s") == 0) {
			if (i + 1 >= argc) { cout << "You must specify max states.\n"; return 0; }
			maxStates = atoi(argv[++i]);
			if (maxStates < 1) { cout << "You must specify a valid max number of states.\n"; return 0; }
		} else if (_stricmp(argv[i], "-mvs") == 0 || _stricmp(argv[i], "/mvs") == 0 || _stricmp(argv[i], "-moves") == 0 || _stricmp(argv[i], "/moves") == 0) {
			showMoves = true;
		} else if (_stricmp(argv[i], "-r") == 0 || _stricmp(argv[i], "/r") == 0) {
			replay = true;
		} else if (_stricmp(argv[i], "-?") == 0 || _stricmp(argv[i], "/?") == 0 || _stricmp(argv[i], "?") == 0 || _stricmp(argv[i], "/help") == 0 || _stricmp(argv[i], "-help") == 0) {
			cout << "SimpleSolver\n";
			cout << "Deals a Klondike game by FreeCell (FC) game number and plays it with a\n";
			cout << "simple greedy method that never draws from stock while another move exists.\n\n";
			cout << "SimpleSolver [/G #] [/DC #] [/S #] [/MVS] [/R] [#]\n\n";
			cout << "  /GAME # [/G #]    FC game number to deal and solve. Defaults to 1.\n";
			cout << "  /DRAW # [/DC #]   Draw count to use. Defaults to 1.\n";
			cout << "  /STATES # [/S #]  Max game states to evaluate. Defaults to 5,000,000.\n";
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
	SolveResult result = s.SolveSimple(maxStates);
	clock_t elapsed = clock() - start;

	bool solved = (result == SolvedMinimal || result == SolvedMayNotBeMinimal);
	if (result == SolvedMinimal) {
		cout << "Solved with the simple method in " << s.MovesMadeNormalizedCount() << " moves (minimal under the no-draw constraint).";
	} else if (result == SolvedMayNotBeMinimal) {
		cout << "Solved with the simple method in " << s.MovesMadeNormalizedCount() << " moves (state limit hit, may not be minimal).";
	} else if (result == Impossible) {
		cout << "Impossible under the simple method. Best foundation count " << s.FoundationCount() << "/52.";
	} else {
		cout << "Could not complete the search (state limit hit). Best foundation count " << s.FoundationCount() << "/52.";
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

#include<iostream>
#include<cstring>
#include<cstdlib>
#include<ctime>
#include"Solitaire.h"
#define _stricmp strcasecmp

using namespace std;

//A variation of the Klondike solver that deals a game by its FreeCell (FC) game
//number and solves it with a constrained exhaustive search. Two constraint sets are
//available:
//
//  simple  (default) - the same full exhaustive search as the minimal solver, with a
//          single added rule: it is not allowed to draw from the stock while any other
//          move is available (a move between tableau columns, a move to a foundation,
//          or a move of the current draw card onto a tableau column).
//
//  simpler (/SIMPLER) - the simple rule plus a second one: when any move onto a
//          foundation is available, only the foundation moves are explored.
//
//  average (/AVERAGE) - a different restriction: drawing from the stock is unrestricted,
//          but the search never takes a card back off a foundation onto the tableau.
//
//In every case the remaining branches are still searched exhaustively, so the result is
//the minimal solution obeying the chosen constraints, or that the deal is impossible
//under them (which can happen even for deals the unconstrained solver can win).

int main(int argc, char * argv[]) {
	Solitaire s;
	s.Initialize();

	int gameNumber = 1;
	int drawCount = 1;
	int maxStates = 5000000;
	bool showMoves = false;
	bool replay = false;
	bool simpler = false;
	bool average = false;

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
		} else if (_stricmp(argv[i], "-simpler") == 0 || _stricmp(argv[i], "/simpler") == 0) {
			simpler = true;
		} else if (_stricmp(argv[i], "-average") == 0 || _stricmp(argv[i], "/average") == 0 || _stricmp(argv[i], "-avg") == 0 || _stricmp(argv[i], "/avg") == 0) {
			average = true;
		} else if (_stricmp(argv[i], "-mvs") == 0 || _stricmp(argv[i], "/mvs") == 0 || _stricmp(argv[i], "-moves") == 0 || _stricmp(argv[i], "/moves") == 0) {
			showMoves = true;
		} else if (_stricmp(argv[i], "-r") == 0 || _stricmp(argv[i], "/r") == 0) {
			replay = true;
		} else if (_stricmp(argv[i], "-?") == 0 || _stricmp(argv[i], "/?") == 0 || _stricmp(argv[i], "?") == 0 || _stricmp(argv[i], "/help") == 0 || _stricmp(argv[i], "-help") == 0) {
			cout << "SimpleSolver\n";
			cout << "Deals a Klondike game by FreeCell (FC) game number and solves it with a\n";
			cout << "constrained exhaustive search that never draws from stock while another move\n";
			cout << "exists (and, with /SIMPLER, only plays to a foundation when it can).\n\n";
			cout << "SimpleSolver [/G #] [/DC #] [/S #] [/SIMPLER] [/AVERAGE] [/MVS] [/R] [#]\n\n";
			cout << "  /GAME # [/G #]    FC game number to deal and solve. Defaults to 1.\n";
			cout << "  /DRAW # [/DC #]   Draw count to use. Defaults to 1.\n";
			cout << "  /STATES # [/S #]  Max game states to evaluate. Defaults to 5,000,000.\n";
			cout << "  /SIMPLER          Also explore only foundation moves when any are available.\n";
			cout << "  /AVERAGE [/AVG]   Exhaustive search that never un-plays a foundation card.\n";
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

	const char * method = average ? "average" : (simpler ? "simpler" : "simple");
	cout << "FC Game #" << gameNumber << " (draw " << drawCount << ", " << method << " method)\n";
	cout << s.GameDiagram() << "\n\n";

	clock_t start = clock();
	SolveResult result = average ? s.SolveAverage(maxStates) : (simpler ? s.SolveSimpler(maxStates) : s.SolveSimple(maxStates));
	clock_t elapsed = clock() - start;

	bool solved = (result == SolvedMinimal || result == SolvedMayNotBeMinimal);
	if (result == SolvedMinimal) {
		cout << "Solved with the " << method << " method in " << s.MovesMadeNormalizedCount() << " moves (minimal under its constraints).";
	} else if (result == SolvedMayNotBeMinimal) {
		cout << "Solved with the " << method << " method in " << s.MovesMadeNormalizedCount() << " moves (state limit hit, may not be minimal).";
	} else if (result == Impossible) {
		cout << "Impossible under the " << method << " method. Best foundation count " << s.FoundationCount() << "/52.";
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

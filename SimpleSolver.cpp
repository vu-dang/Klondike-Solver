#include<iostream>
#include<cstring>
#include<cstdlib>
#include<ctime>
#include"Solitaire.h"
#define _stricmp strcasecmp

using namespace std;

//A variation of the Klondike solver that deals a game by its FreeCell (FC) game
//number and solves it with a constrained exhaustive search. Three constraint sets are
//available:
//
//  intermediate (default) - the same full exhaustive search as the minimal solver, with
//          a single added rule: it is not allowed to draw from the stock while any other
//          move is available (a move between tableau columns, a move to a foundation,
//          or a move of the current draw card onto a tableau column).
//
//  beginner (/BEGINNER) - the intermediate rule plus a second one: when any move onto a
//          foundation is available, only the foundation moves are explored.
//
//  expert (/EXPERT) - a different restriction: drawing from the stock is unrestricted,
//          but the search never takes a card back off a foundation onto the tableau.
//
//In every case the remaining branches are still searched exhaustively, so the result is
//the minimal solution obeying the chosen constraints, or that the deal is impossible
//under them (which can happen even for deals the unconstrained solver can win).

int main(int argc, char * argv[]) {
	Solitaire s;
	s.Initialize();

	int gameNumber = 1;
	int toGame = 0;
	int drawCount = 1;
	int maxStates = 5000000;
	bool showMoves = false;
	bool replay = false;
	bool beginner = false;
	bool expert = false;

	for (int i = 1; i < argc; i++) {
		if (_stricmp(argv[i], "-game") == 0 || _stricmp(argv[i], "/game") == 0 || _stricmp(argv[i], "-g") == 0 || _stricmp(argv[i], "/g") == 0) {
			if (i + 1 >= argc) { cout << "You must specify an FC game number to load.\n"; return 0; }
			gameNumber = atoi(argv[++i]);
		} else if (_stricmp(argv[i], "-togame") == 0 || _stricmp(argv[i], "/togame") == 0 || _stricmp(argv[i], "-tg") == 0 || _stricmp(argv[i], "/tg") == 0) {
			if (i + 1 >= argc) { cout << "You must specify an FC game number to solve up to.\n"; return 0; }
			toGame = atoi(argv[++i]);
		} else if (_stricmp(argv[i], "-draw") == 0 || _stricmp(argv[i], "/draw") == 0 || _stricmp(argv[i], "-dc") == 0 || _stricmp(argv[i], "/dc") == 0) {
			if (i + 1 >= argc) { cout << "You must specify draw count.\n"; return 0; }
			drawCount = atoi(argv[++i]);
			if (drawCount < 1 || drawCount > 12) { cout << "Please specify a valid draw count from 1 to 12.\n"; return 0; }
		} else if (_stricmp(argv[i], "-states") == 0 || _stricmp(argv[i], "/states") == 0 || _stricmp(argv[i], "-s") == 0 || _stricmp(argv[i], "/s") == 0) {
			if (i + 1 >= argc) { cout << "You must specify max states.\n"; return 0; }
			maxStates = atoi(argv[++i]);
			if (maxStates < 1) { cout << "You must specify a valid max number of states.\n"; return 0; }
		} else if (_stricmp(argv[i], "-beginner") == 0 || _stricmp(argv[i], "/beginner") == 0) {
			beginner = true;
		} else if (_stricmp(argv[i], "-expert") == 0 || _stricmp(argv[i], "/expert") == 0 || _stricmp(argv[i], "-exp") == 0 || _stricmp(argv[i], "/exp") == 0) {
			expert = true;
		} else if (_stricmp(argv[i], "-mvs") == 0 || _stricmp(argv[i], "/mvs") == 0 || _stricmp(argv[i], "-moves") == 0 || _stricmp(argv[i], "/moves") == 0) {
			showMoves = true;
		} else if (_stricmp(argv[i], "-r") == 0 || _stricmp(argv[i], "/r") == 0) {
			replay = true;
		} else if (_stricmp(argv[i], "-?") == 0 || _stricmp(argv[i], "/?") == 0 || _stricmp(argv[i], "?") == 0 || _stricmp(argv[i], "/help") == 0 || _stricmp(argv[i], "-help") == 0) {
			cout << "SimpleSolver\n";
			cout << "Deals a Klondike game by FreeCell (FC) game number and solves it with a\n";
			cout << "constrained exhaustive search. Defaults to the intermediate method, which never\n";
			cout << "draws from stock while another move exists.\n\n";
			cout << "SimpleSolver [/G #] [/DC #] [/S #] [/BEGINNER] [/EXPERT] [/MVS] [/R] [#]\n\n";
			cout << "  /GAME # [/G #]    FC game number to deal and solve. Defaults to 1.\n";
		cout << "  /TOGAME # [/TG #] Solve a range of games from /GAME to this number,\n";
		cout << "                    printing CSV lines (game,moves,ms) for each solved deal.\n";
			cout << "  /DRAW # [/DC #]   Draw count to use. Defaults to 1.\n";
			cout << "  /STATES # [/S #]  Max game states to evaluate. Defaults to 5,000,000.\n";
			cout << "  /BEGINNER         Intermediate rule, plus explore only foundation moves when any exist.\n";
			cout << "  /EXPERT [/EXP]    Exhaustive search that never un-plays a foundation card.\n";
			cout << "  /MOVES [/MVS]     Output the compact list of moves made when solved.\n";
			cout << "  /R                Replay the solution step by step to output.\n";
			cout << "  #                 A bare number is treated as the FC game number.\n";
			return 0;
		} else {
			gameNumber = atoi(argv[i]);
		}
	}

	s.SetDrawCount(drawCount);

	//Range mode: when /TOGAME is given, solve every deal from gameNumber to toGame
	//and emit one CSV line (game,moves,ms) per solved deal, like KlondikeSolver.
	if (toGame > 0) {
		int last = toGame < gameNumber ? gameNumber : toGame;
		printf("game,moves,time_us,num_recycles,max_states\n");
		for (int deal = gameNumber; deal <= last; deal++) {
			s.ShuffleFC(deal);
			s.ResetGame(drawCount);
			clock_t start = clock();
			SolveResult result = expert ? s.SolveExpert(maxStates) : (beginner ? s.SolveBeginner(maxStates) : s.SolveIntermediate(maxStates));
			clock_t elapsed = clock() - start;
			//Print a row for every deal; unsolved deals report 0 moves.
			bool solved = (result == SolvedMinimal || result == SolvedMayNotBeMinimal);
			int moves = solved ? s.MovesMadeNormalizedCount() : 0;
			printf("%d,%d,%lu,%d,%d\n", deal, moves, (unsigned long)elapsed, s.RoundCount(), s.StatesUsed());
		}
		return 0;
	}

	s.ShuffleFC(gameNumber);
	s.ResetGame(drawCount);

	const char * method = expert ? "expert" : (beginner ? "beginner" : "intermediate");
	cout << "FC Game #" << gameNumber << " (draw " << drawCount << ", " << method << " method)\n";
	cout << s.GameDiagram() << "\n\n";

	clock_t start = clock();
	SolveResult result = expert ? s.SolveExpert(maxStates) : (beginner ? s.SolveBeginner(maxStates) : s.SolveIntermediate(maxStates));
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

#include<iostream>
#include<cstring>
#include<cstdlib>
#include<ctime>
#include"Solitaire.h"
#define _stricmp strcasecmp

using namespace std;

//A variation of the Klondike solver that deals a game by its FreeCell (FC) game
//number and solves it. By default it runs the same full, unconstrained exhaustive
//search as the minimal solver. Three optional constraint sets are also available:
//
//  simple (/SIMPLE) - the unconstrained exhaustive search with a single added rule:
//          it is not allowed to draw from the stock while any other move is available
//          (a move between tableau columns, a move to a foundation, or a move of the
//          current draw card onto a tableau column).
//
//  beginner (/BEGINNER) - the simple rule plus a second one: when any move onto a
//          foundation is available, only the foundation moves are explored.
//
//  expert (/EXPERT) - a different restriction: drawing from the stock is unrestricted,
//          but the search never takes a card back off a foundation onto the tableau.
//
//In every case the remaining branches are still searched exhaustively, so the result is
//the minimal solution obeying the chosen constraints, or that the deal is impossible
//under them (which can happen even for deals the unconstrained solver can win).
//
//The constraint set is chosen with /CONSTRAINT <mode>, where <mode> is one of
//beginner, simple, expert, or stepup. With no /CONSTRAINT the unconstrained minimal
//solver is used. "stepup" escalates beginner -> simple -> expert -> unconstrained,
//stopping at the first mode that solves the deal.

static bool IsSolved(SolveResult result) {
	return result == SolvedMinimal || result == SolvedMayNotBeMinimal;
}

//Resets the dealt game and runs the single solver matching mode. Anything other than
//beginner/simple/expert runs the unconstrained minimal solver. numThreads > 1 uses the
//multithreaded minimal solver, which only exists for the unconstrained mode; the
//constrained solvers always run single-threaded.
static SolveResult RunSolve(Solitaire & s, int drawCount, int maxStates, const char * mode, int numThreads) {
	s.ResetGame(drawCount);
	if (_stricmp(mode, "beginner") == 0) { return s.SolveBeginner(maxStates); }
	if (_stricmp(mode, "simple") == 0) { return s.SolveIntermediate(maxStates); }
	if (_stricmp(mode, "expert") == 0) { return s.SolveExpert(maxStates); }
	return numThreads > 1 ? s.SolveMinimalMultithreaded(numThreads, maxStates) : s.SolveMinimal(maxStates);
}

//Solves the already-dealt game under the requested constraint. For "stepup" it escalates
//beginner -> simple -> expert -> unconstrained, stopping at the first mode that solves.
//Sets usedMode to the mode that produced the returned result.
static SolveResult SolveConstraint(Solitaire & s, int drawCount, int maxStates, const char * constraint, int numThreads, const char * & usedMode) {
	if (_stricmp(constraint, "stepup") == 0) {
		static const char * ladder[] = { "beginner", "simple", "expert", "unconstrained" };
		SolveResult result = Impossible;
		for (int i = 0; i < 4; i++) {
			result = RunSolve(s, drawCount, maxStates, ladder[i], numThreads);
			usedMode = ladder[i];
			if (IsSolved(result)) { return result; }
		}
		return result;
	}
	usedMode = constraint;
	return RunSolve(s, drawCount, maxStates, constraint, numThreads);
}

int main(int argc, char * argv[]) {
	Solitaire s;
	s.Initialize();

	int gameNumber = 1;
	int toGame = 0;
	int drawCount = 1;
	int maxStates = 5000000;
	bool showMoves = false;
	bool replay = false;
	int multiThreaded = 1;
	const char * constraint = "unconstrained";
	const char * outputFile = NULL;

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
		} else if (_stricmp(argv[i], "-constraint") == 0 || _stricmp(argv[i], "/constraint") == 0 || _stricmp(argv[i], "-c") == 0 || _stricmp(argv[i], "/c") == 0) {
			if (i + 1 >= argc) { cout << "You must specify a constraint: beginner, simple, expert, or stepup.\n"; return 0; }
			constraint = argv[++i];
			if (_stricmp(constraint, "beginner") != 0 && _stricmp(constraint, "simple") != 0 && _stricmp(constraint, "expert") != 0 && _stricmp(constraint, "stepup") != 0) {
				cout << "Invalid constraint '" << constraint << "'. Use beginner, simple, expert, or stepup.\n"; return 0;
			}
		} else if (_stricmp(argv[i], "-m") == 0 || _stricmp(argv[i], "/m") == 0 || _stricmp(argv[i], "-multi") == 0 || _stricmp(argv[i], "/multi") == 0) {
			if (i + 1 >= argc) { cout << "You must specify number of threads.\n"; return 0; }
			multiThreaded = atoi(argv[++i]);
			if (multiThreaded < 2 || multiThreaded > 99) { cout << "Please specify a valid number of threads from 2 to 99.\n"; return 0; }
		} else if (_stricmp(argv[i], "-output") == 0 || _stricmp(argv[i], "/output") == 0 || _stricmp(argv[i], "-o") == 0 || _stricmp(argv[i], "/o") == 0) {
			if (i + 1 >= argc) { cout << "You must specify an output filename.\n"; return 0; }
			outputFile = argv[++i];
		} else if (_stricmp(argv[i], "-mvs") == 0 || _stricmp(argv[i], "/mvs") == 0 || _stricmp(argv[i], "-moves") == 0 || _stricmp(argv[i], "/moves") == 0) {
			showMoves = true;
		} else if (_stricmp(argv[i], "-r") == 0 || _stricmp(argv[i], "/r") == 0) {
			replay = true;
		} else if (_stricmp(argv[i], "-?") == 0 || _stricmp(argv[i], "/?") == 0 || _stricmp(argv[i], "?") == 0 || _stricmp(argv[i], "/help") == 0 || _stricmp(argv[i], "-help") == 0) {
			cout << "SimpleSolver\n";
			cout << "Deals a Klondike game by FreeCell (FC) game number and solves it. By default it\n";
			cout << "runs a full exhaustive (unconstrained) search; a constraint set can be selected\n";
			cout << "with /CONSTRAINT.\n\n";
			cout << "SimpleSolver [/G #] [/DC #] [/S #] [/CONSTRAINT mode] [/M #] [/OUTPUT file] [/MVS] [/R] [#]\n\n";
			cout << "  /GAME # [/G #]    FC game number to deal and solve. Defaults to 1.\n";
			cout << "  /TOGAME # [/TG #] Solve a range of games from /GAME to this number,\n";
			cout << "                    printing CSV lines per deal.\n";
			cout << "  /DRAW # [/DC #]   Draw count to use. Defaults to 1.\n";
			cout << "  /STATES # [/S #]  Max game states to evaluate. Defaults to 5,000,000.\n";
			cout << "  /CONSTRAINT mode [/C mode]  Constraint set to impose. One of:\n";
			cout << "                      beginner - simple rule, plus explore only foundation moves when any exist.\n";
			cout << "                      simple   - never draw from stock while another move exists.\n";
			cout << "                      expert   - exhaustive search that never un-plays a foundation card.\n";
			cout << "                      stepup   - try beginner, then simple, then expert, then unconstrained.\n";
			cout << "                    Omit /CONSTRAINT to run the unconstrained minimal solver.\n";
			cout << "  /MULTI # [/M #]   Use # threads (2-99) for the unconstrained minimal solve.\n";
			cout << "                    Ignored by the constrained solvers, which are single-threaded.\n";
			cout << "  /OUTPUT file [/O file]  Write the CSV output to file (created in the current\n";
			cout << "                    directory) instead of stdout. Only applies in /TOGAME range mode.\n";
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
		//Write CSV to the file given by /OUTPUT, or to stdout when none was specified.
		FILE * out = stdout;
		if (outputFile != NULL) {
			out = fopen(outputFile, "w");
			if (out == NULL) { cout << "Could not open output file '" << outputFile << "' for writing.\n"; return 0; }
			cout << "Writing CSV output to " << outputFile << "\n";
		}
		fprintf(out, "game,moves,time_us,num_recycles,max_states,constraint\n");
		for (int deal = gameNumber; deal <= last; deal++) {
			s.ShuffleFC(deal);
			const char * usedMode;
			clock_t start = clock();
			SolveResult result = SolveConstraint(s, drawCount, maxStates, constraint, multiThreaded, usedMode);
			clock_t elapsed = clock() - start;
			//Print a row for every deal; unsolved deals report 0 moves.
			int moves = IsSolved(result) ? s.MovesMadeNormalizedCount() : 0;
			fprintf(out, "%d,%d,%lu,%d,%d,%s\n", deal, moves, (unsigned long)elapsed, s.RoundCount(), s.StatesUsed(), usedMode);
			fflush(out);
		}
		if (out != stdout) { fclose(out); }
		return 0;
	}

	s.ShuffleFC(gameNumber);
	s.ResetGame(drawCount);

	cout << "FC Game #" << gameNumber << " (draw " << drawCount << ", " << constraint << " constraint)\n";
	cout << s.GameDiagram() << "\n\n";

	const char * method;
	clock_t start = clock();
	SolveResult result = SolveConstraint(s, drawCount, maxStates, constraint, multiThreaded, method);
	clock_t elapsed = clock() - start;

	bool solved = IsSolved(result);
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
